#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// HWND = Handle to a window
// Also temporarily here for testing
static HWND g_hwnd;
static int g_running = 1;

//static GamepadState controllers[MAX_CONTROLLERS];

static LRESULT CALLBACK RawInputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    printf("WndProc msg: 0x%X\n", msg);
    fflush(stdout);
    switch (msg)
    {
        case WM_INPUT:
        {
            printf("WM_INPUT received\n");
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT,NULL, &size, sizeof(RAWINPUTHEADER));
            BYTE buffer[256];

            if (size > sizeof(buffer)) {
                printf("ERROR: size is bigger than buffer");
                break;
            }
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) != size) {
                printf("ERROR: Raw Input data is not the same size as buffer");
                break;
            }

            // Initialize rawInput to our buffer
            RAWINPUT* raw = (RAWINPUT*)buffer;

            if (raw->header.dwType == RIM_TYPEHID)
            {
                RAWHID* hid = &raw->data.hid;

                // I just now want the controller report
                printf("HID report: %u bytes\n", hid->dwSizeHid);
                fflush(stdout);

                // TODO: Parse controller Info
            }

            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int raw_init() {
    // Collect data needed for Window
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = RawInputWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "RawInputHiddenWindow";

    if (!RegisterClass(&wc)) {
        printf("RegisterClass Failed: %lu\n", GetLastError());
        return 0;
    }
    printf("RegisterClass OK\n");

    // Create our window with no parameters so Win32 can send the HID messages to us in background
    g_hwnd = CreateWindowEx(
        0, 
        wc.lpszClassName, 
        "", 
        0, 
        0, 0, 0, 0, 
        NULL, 
        NULL, 
        hInstance, 
        NULL
    );

    if (!g_hwnd) {
        printf("CreateWindowEx failed: %lu\n", GetLastError());
        return 0;
    }
    printf("Hidden window created: %p\n", g_hwnd);

    // Register raw input device as a generic gamepad
     // For now only look for one controller
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    rid.usUsage = 0x05;              // HID_USAGE_GENERIC_GAMEPAD
    rid.dwFlags = RIDEV_INPUTSINK;   // holds controller input even when unfocused to window
    rid.hwndTarget = g_hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        printf("RegisterRawInputDevices failed: %lu\n", GetLastError());
        return 0;
    }
    printf("RegisterRawInputDevices OK\n");

    return 1;
}

int main() {
    if (!raw_init()) {
        printf("Raw Init Failed\n");
        return 1;
    }
    printf("RawInput initialized. Waiting for input...\n");

    while (g_running) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(1);
    }
    return 0;
}