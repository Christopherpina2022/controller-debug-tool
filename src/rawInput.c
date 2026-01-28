#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <RawInput_Backend.h>
#include <hidProfiles.h>

typedef struct {
    uint8_t  buttonDown[GAMEPAD_BUTTON_COUNT]; // 1 for pressed, 0 for released
    float    axisValue[GAMEPAD_MAX_AXES];
} RawState;

static GamepadState gState[MAX_CONTROLLERS];
static HidRecord hidRecord[MAX_CONTROLLERS];
static int hidDevCount = 0;
static HWND g_hwnd;

void parseReport(HidRecord *dev, const BYTE *report, UINT size, int devIndex) {
    RawState rState = {0};
    if (devIndex < 0 || devIndex >= MAX_CONTROLLERS)
        return;

    // Button control
    for (USHORT i = 0; i < dev->buttonCapCount; i++) {
        HIDP_BUTTON_CAPS *bc = &dev->buttonCaps[i];
        USAGE usages[32];
        ULONG usageCount = 32;

        if (HidP_GetUsages(
                HidP_Input,
                bc->UsagePage,
                0,
                usages,
                &usageCount,
                dev->preparsed,
                (PCHAR)report,
                size
            ) != HIDP_STATUS_SUCCESS) {
            continue;
        }

        for (ULONG u = 0; u < usageCount; u++) {
            USAGE usage = usages[u];
            if (usage < MAX_USAGES) {
                int btn = dev->buttonMap[usage];
                if (btn >= 0)
                    rState.buttonDown[btn] = 1;
            }
        }
    }

    // Axis control
    for (USHORT i = 0; i < dev->valueCapCount; i++) {
        HIDP_VALUE_CAPS *vc = &dev->valueCaps[i];
        LONG value;

        if (HidP_GetUsageValue(
                HidP_Input,
                vc->UsagePage,
                0,
                vc->NotRange.Usage,
                &value,
                dev->preparsed,
                (PCHAR)report,
                size
            ) != HIDP_STATUS_SUCCESS) {
            continue;
        }

        USAGE usage = vc->NotRange.Usage;
        if (usage >= MAX_USAGES)
            continue;

        int axis = dev->axisMap[usage];
        if (axis < 0 || axis >= GAMEPAD_MAX_AXES)
            continue;

        float norm = 0.0f;
        if (vc->LogicalMax != vc->LogicalMin) {
            norm = (float)(value - vc->LogicalMin) /
                (float)(vc->LogicalMax - vc->LogicalMin);
            norm = norm * 2.0f - 1.0f;
        }

        rState.axisValue[axis] = norm;

        for (USHORT i = 0; i < dev->valueCapCount; i++) {
            HIDP_VALUE_CAPS *vc = &dev->valueCaps[i];

            printf(
                "UsagePage 0x%02X Usage 0x%02X | Logical [%ld, %ld]\n",
                vc->UsagePage,
                vc->NotRange.Usage,
                vc->LogicalMin,
                vc->LogicalMax
            );
        }
    }

    // Update frontend GamepadState directly
    GamepadState *g = &gState[devIndex];
    memset(g, 0, sizeof(*g));
    g->connected = 1;

    for (int i = 0; i < GAMEPAD_BUTTON_COUNT; i++) {
        if (rState.buttonDown[i])
            g->buttons |= (1 << i);
    }

    for (int i = 0; i < GAMEPAD_MAX_AXES; i++) {
        g->axes[i] = rState.axisValue[i];
    }
}

HidRecord *devReg(HANDLE hDevice) {
    // Check first if device was already registered
    for (int i = 0; i < hidDevCount; i++) {
        if (hidRecord[i].device == hDevice)
        {
            return &hidRecord[i];
        }
    }

    // Safety check if the hidRecord count is above our max
    if (hidDevCount >= MAX_CONTROLLERS) {
        return NULL;
    }

    HidRecord *newRecord = &hidRecord[hidDevCount++];
    memset(newRecord, 0, sizeof(HidRecord));
    newRecord->device = hDevice;

    RID_DEVICE_INFO info;
    UINT infoSize = sizeof(info);
    info.cbSize = sizeof(info);

    // Assign vendor ID and Product ID
    if (GetRawInputDeviceInfo(
        hDevice,
        RIDI_DEVICEINFO,
        &info,
        &infoSize
    ) > 0 ) {
        if (info.dwType == RIM_TYPEHID) {
            newRecord->vendorID = info.hid.dwVendorId;
            newRecord->productID = info.hid.dwProductId;
        }
    }

    // clear maps
    for (int i = 0; i < MAX_USAGES; i++) {
        newRecord->buttonMap[i] = -1;
        newRecord->axisMap[i]   = -1;
    }

    // Get preparsed data
    UINT size = 0;
    GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &size);
    newRecord->preparsed = malloc(size);
    GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, newRecord->preparsed, &size);

    // Get caps
    HidP_GetCaps(newRecord->preparsed, &newRecord->caps);

    newRecord->buttonCapCount = newRecord->caps.NumberInputButtonCaps;
    newRecord->valueCapCount  = newRecord->caps.NumberInputValueCaps;

    // Allocate caps
    newRecord->buttonCaps = malloc(sizeof(HIDP_BUTTON_CAPS) * newRecord->buttonCapCount);
    newRecord->valueCaps  = malloc(sizeof(HIDP_VALUE_CAPS)  * newRecord->valueCapCount);

    // Fill capabilities
    HidP_GetButtonCaps(HidP_Input, newRecord->buttonCaps,
                    &newRecord->buttonCapCount, newRecord->preparsed);

    HidP_GetValueCaps(HidP_Input, newRecord->valueCaps,
                    &newRecord->valueCapCount, newRecord->preparsed);
    
    // Values not mapped by our profile function will show NULL
    for (int i = 0; i < MAX_USAGES; i++) {
        newRecord->buttonMap[i] = -1;
        newRecord->axisMap[i] = -1;
    }

    // Map buttons to our inputs
    //applyProfile(newRecord);

    return newRecord;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INPUT: {
            BYTE buffer[1024];
            UINT size = 0;

            // Queries the required size before we put into stack
            if (GetRawInputData(
                (HRAWINPUT)lParam, 
                RID_INPUT, 
                NULL, 
                &size, 
                sizeof(RAWINPUTHEADER)
            ) == (UINT)-1) {
                break;
            }
            
            if (size > sizeof(buffer)) {
                break;
            }

            // Assign Raw Input data to our stack
            UINT read = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));
            if (read == (UINT)-1 || read != size) {
                break;
            }

            // Load RawInput struct with our input data
            RAWINPUT *raw = (RAWINPUT *)buffer;

            if (raw->header.dwType == RIM_TYPEHID) {
                HidRecord *dev = devReg(raw->header.hDevice);
                if (dev) {
                    // Parse the report
                    int devIndex = (int)(dev - hidRecord);
                    for (DWORD i = 0; i < raw->data.hid.dwCount; i++) {
                        const BYTE *report = raw->data.hid.bRawData + (i * raw->data.hid.dwSizeHid);
                        
                        parseReport(dev, report, raw->data.hid.dwSizeHid, devIndex);
                    }
                }
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void rawInit() {
    memset(gState, 0, sizeof(gState));
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RawInputWindow";

    if (!RegisterClass(&wc)) {
        printf("RegisterClass failed: %lu\n", GetLastError());
        return;
    }

    g_hwnd = CreateWindowEx(0, wc.lpszClassName, "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!g_hwnd) {
        printf("CreateWindowEx failed: %lu\n", GetLastError());
        return;
    }

    RAWINPUTDEVICE rid[3];

    rid[0] = (RAWINPUTDEVICE){
        .usUsagePage = 0x01,
        .usUsage     = 0x04,   // Joystick
        .dwFlags     = RIDEV_INPUTSINK,
        .hwndTarget  = g_hwnd
    };

    rid[1] = (RAWINPUTDEVICE){
        .usUsagePage = 0x01,
        .usUsage     = 0x05,   // Gamepad
        .dwFlags     = RIDEV_INPUTSINK,
        .hwndTarget  = g_hwnd
    };

    rid[2] = (RAWINPUTDEVICE){
        .usUsagePage = 0x01,
        .usUsage     = 0x08,   // Multi-axis
        .dwFlags     = RIDEV_INPUTSINK,
        .hwndTarget  = g_hwnd
    };

    if (RegisterRawInputDevices(rid, 3, sizeof(RAWINPUTDEVICE)) == FALSE) {
        printf("Registration Failed: %lu\n", GetLastError());
        return;
    }
}

void rawUpdate() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

const GamepadState *rawinput_get_gamepad(int index) {
    if (index < 0 || index >= MAX_CONTROLLERS)
        return NULL;
    return &gState[index];
}