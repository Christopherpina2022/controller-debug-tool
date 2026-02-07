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

void parseReport(HidRecord *dev, const BYTE *report, UINT size) {
    GamepadState *g = dev->state;
    g->connected = 1;
    memset(g->axes, 0, sizeof(g->axes));
    g->buttons = 0;

    // Loop through DB-mapped axes
    for (int i = 0; i < dev->axisCount; i++) {
        AxisMapping *map = &dev->axes[i];

        // Skip if we couldn't find capIndex
        if (map->capIndex < 0 || map->capIndex >= dev->valueCapCount)
            continue;

        HIDP_VALUE_CAPS *vc = &dev->valueCaps[map->capIndex];
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
            ) != HIDP_STATUS_SUCCESS)
            continue;

        // Normalize to [-1, 1]
        float norm = (float)(value - vc->LogicalMin) / (float)(vc->LogicalMax - vc->LogicalMin);
        norm = norm * 2.0f - 1.0f;

        g->axes[map->mappedEnum] = applyDeadzone(norm, DEADZONE);
    }

    // Loop through DB-mapped Buttons
    ULONG usageCount = 32;
    USAGE usages[32];

    if (HidP_GetUsages(
            HidP_Input,
            HID_USAGE_PAGE_BUTTON,
            0,
            usages,
            &usageCount,
            dev->preparsed,
            (PCHAR)report,
            size
        ) == HIDP_STATUS_SUCCESS) {

        for (ULONG i = 0; i < usageCount; i++) {
            for (int b = 0; b < dev->buttonCount; b++) {
                if (dev->buttons[b].usage == usages[i]) {
                    g->buttons |= (1u << dev->buttons[b].mappedEnum);
                }
            }
        }
    }

    // Loop through DB-mapped DPAD mapping
    if (dev->hatCapIndex >= 0) {
        HIDP_VALUE_CAPS *vc = &dev->valueCaps[dev->hatCapIndex];
        LONG hat;

        if (HidP_GetUsageValue(
                HidP_Input,
                vc->UsagePage,
                0,
                vc->NotRange.Usage,
                &hat,
                dev->preparsed,
                (PCHAR)report,
                size
            ) == HIDP_STATUS_SUCCESS) {

            // Clear DPAD bits first
            g->buttons &= ~(BTN_DPAD_UP | BTN_DPAD_DOWN |
                BTN_DPAD_LEFT | BTN_DPAD_RIGHT);

            switch (hat) {
                case 0: g->buttons |= BTN_DPAD_UP; break;
                case 1: g->buttons |= BTN_DPAD_UP | BTN_DPAD_RIGHT; break;
                case 2: g->buttons |= BTN_DPAD_RIGHT; break;
                case 3: g->buttons |= BTN_DPAD_DOWN | BTN_DPAD_RIGHT; break;
                case 4: g->buttons |= BTN_DPAD_DOWN; break;
                case 5: g->buttons |= BTN_DPAD_DOWN | BTN_DPAD_LEFT; break;
                case 6: g->buttons |= BTN_DPAD_LEFT; break;
                case 7: g->buttons |= BTN_DPAD_UP | BTN_DPAD_LEFT; break;
                default: break; // neutral
            }
        }
    }
}


HidRecord *devReg(HANDLE hDevice) {
    // Return existing record if already registered
    for (int i = 0; i < hidDevCount; i++) {
        if (hidRecord[i].device == hDevice)
            return &hidRecord[i];
    }

    if (hidDevCount >= MAX_CONTROLLERS)
        return NULL;

    HidRecord *dev = &hidRecord[hidDevCount++];
    memset(dev, 0, sizeof(HidRecord));
    dev->device = hDevice;

    // Get VID/PID
    RID_DEVICE_INFO info;
    UINT infoSize = sizeof(info);
    info.cbSize = sizeof(info);
    if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &info, &infoSize) > 0) {
        if (info.dwType == RIM_TYPEHID) {
            dev->vendorID = info.hid.dwVendorId;
            dev->productID = info.hid.dwProductId;
        }
    }

    // Preparsed data
    UINT size = 0;
    GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &size);
    dev->preparsed = malloc(size);
    GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, dev->preparsed, &size);

    // Capabilities
    HidP_GetCaps(dev->preparsed, &dev->caps);
    dev->buttonCapCount = dev->caps.NumberInputButtonCaps;
    dev->valueCapCount  = dev->caps.NumberInputValueCaps;

    dev->buttonCaps = malloc(sizeof(HIDP_BUTTON_CAPS) * dev->buttonCapCount);
    dev->valueCaps  = malloc(sizeof(HIDP_VALUE_CAPS)  * dev->valueCapCount);

    HidP_GetButtonCaps(HidP_Input, dev->buttonCaps, &dev->buttonCapCount, dev->preparsed);
    HidP_GetValueCaps(HidP_Input, dev->valueCaps, &dev->valueCapCount, dev->preparsed);

    // Identify axes based on Generic Desktop UsagePage
    dev->axisCount = 0;
    for (USHORT i = 0; i < dev->valueCapCount; i++) {
        HIDP_VALUE_CAPS *vc = &dev->valueCaps[i];
        if (vc->UsagePage == HID_USAGE_PAGE_GENERIC_DESKTOP) {
            dev->axisCapIndex[dev->axisCount++] = i;
        }
    }

    // buildHIDMap fills dev->axes[i].mappedEnum and dev->axes[i].usage
    buildHIDMap(dev);

    // Map Axes
    for (int i = 0; i < dev->axisCount; i++) {
        AxisMapping *map = &dev->axes[i];
        map->capIndex = -1;

        for (USHORT j = 0; j < dev->valueCapCount; j++) {
            HIDP_VALUE_CAPS *vc = &dev->valueCaps[j];
            if (vc->UsagePage != HID_USAGE_PAGE_GENERIC_DESKTOP)
                continue;

            // Match DB usage to RawInput usage
            if (vc->NotRange.Usage == map->usage) {
                map->capIndex = j;
                break;
            }
        }
    }

    // Map Buttons
    for (int i = 0; i < dev->buttonCount; i++) {
        ButtonMapping *m = &dev->buttons[i];
        m->usage = 0;

        for (USHORT j = 0; j < dev->buttonCapCount; j++) {
            HIDP_BUTTON_CAPS *bc = &dev->buttonCaps[j];

            if (bc->UsagePage != HID_USAGE_PAGE_BUTTON)
                continue;

            if (bc->IsRange) {
                USHORT usage = bc->Range.UsageMin + m->buttonIndex;
                if (usage <= bc->Range.UsageMax) {
                    m->usage = usage;
                    break;
                }
            }
        }
    }

    // Map DPAD
    dev->hatCapIndex = -1;

    for (USHORT i = 0; i < dev->valueCapCount; i++) {
        HIDP_VALUE_CAPS *vc = &dev->valueCaps[i];

        if (vc->UsagePage == HID_USAGE_PAGE_GENERIC_DESKTOP &&
            vc->NotRange.Usage == HID_USAGE_HAT_SWITCH) {
            dev->hatCapIndex = i;
        }
    }

    dev->state = NULL;
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        if (!gState[i].connected) {
            dev->state = &gState[i];
            break;
        }
    }
    return dev;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INPUT_DEVICE_CHANGE: {
            HANDLE hDevice = (HANDLE)lParam;

            if (wParam == GIDC_REMOVAL) {
                for (int i = 0; i < hidDevCount; i++) {
                    if (hidRecord[i].device == hDevice) {

                        // Mark state disconnected
                        hidRecord[i].state->connected = 0;

                        // Free resources
                        free(hidRecord[i].preparsed);
                        free(hidRecord[i].buttonCaps);
                        free(hidRecord[i].valueCaps);

                        // Compact array (swap with last)
                        hidRecord[i] = hidRecord[hidDevCount - 1];
                        hidDevCount--;

                        break;
                    }
                }
            }
            break;
        }
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
                        
                        parseReport(dev, report, raw->data.hid.dwSizeHid);
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
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        gState[i].connected = 0;
    }
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