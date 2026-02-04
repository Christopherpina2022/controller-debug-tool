#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <RawInput_Backend.h>
#include <hidProfiles.h>

static GamepadState gState[MAX_CONTROLLERS];
static HidRecord hidRecord[MAX_CONTROLLERS];
static int hidDevCount = 0;
static HWND g_hwnd;

static inline float applyDeadzone(float v, float dz) {
    if (fabsf(v) < dz)
        return 0.0f;

    // Rescale so we still reach ±1
    if (v > 0.0f)
        return (v - dz) / (1.0f - dz);
    else
        return (v + dz) / (1.0f - dz);
}

void parseReport(HidRecord *dev, const BYTE *report, UINT size, int devIndex) {
    if (devIndex < 0 || devIndex >= MAX_CONTROLLERS)
        return;

    GamepadState *g = &gState[devIndex];
    g->connected = 1;

    memset(g->axes, 0, sizeof(g->axes));
    g->buttons &= ~(
        (1 << GAMEPAD_BTN_DPAD_UP) |
        (1 << GAMEPAD_BTN_DPAD_DOWN) |
        (1 << GAMEPAD_BTN_DPAD_LEFT) |
        (1 << GAMEPAD_BTN_DPAD_RIGHT)
    );

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
                size) != HIDP_STATUS_SUCCESS)
            continue;

        // DPAD (Hat Switch)
        if (vc->UsagePage == 0x01 && vc->NotRange.Usage == 0x39) {
            switch (value) {
                case 0: g->buttons |= (1 << GAMEPAD_BTN_DPAD_UP); break;
                case 1: g->buttons |= (1 << GAMEPAD_BTN_DPAD_UP) | (1 << GAMEPAD_BTN_DPAD_RIGHT); break;
                case 2: g->buttons |= (1 << GAMEPAD_BTN_DPAD_RIGHT); break;
                case 3: g->buttons |= (1 << GAMEPAD_BTN_DPAD_DOWN) | (1 << GAMEPAD_BTN_DPAD_RIGHT); break;
                case 4: g->buttons |= (1 << GAMEPAD_BTN_DPAD_DOWN); break;
                case 5: g->buttons |= (1 << GAMEPAD_BTN_DPAD_DOWN) | (1 << GAMEPAD_BTN_DPAD_LEFT); break;
                case 6: g->buttons |= (1 << GAMEPAD_BTN_DPAD_LEFT); break;
                case 7: g->buttons |= (1 << GAMEPAD_BTN_DPAD_UP) | (1 << GAMEPAD_BTN_DPAD_LEFT); break;
                default: break; // neutral
            }
            continue;
        }

        // Axes
        if (vc->UsagePage != 0x01)
            continue;

        int axis = vc->NotRange.Usage - 0x30;
        if (axis < 0 || axis >= AXIS_COUNT)
            continue;

        float normalized = 0.0f;
        if (vc->LogicalMax != vc->LogicalMin) {
            normalized = (float)(value - vc->LogicalMin) / (float)(vc->LogicalMax - vc->LogicalMin);
            // Sticks -> -1..1
            normalized = normalized * 2.0f - 1.0f;
        }
        // Apply a deadzone to each stick Axes
        g->axes[AXIS_LX] = applyDeadzone(g->axes[AXIS_LX], 0.2f);
        g->axes[AXIS_LY] = applyDeadzone(g->axes[AXIS_LY], 0.2f);
        g->axes[AXIS_RX] = applyDeadzone(g->axes[AXIS_RX], 0.2f);
        g->axes[AXIS_RY] = applyDeadzone(g->axes[AXIS_RY], 0.2f);

        g->axes[axis] = normalized;
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