#include <windows.h>
#include <stdio.h>

static HWND g_hwnd;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INPUT: {
            printf("WM_INPUT received!\n");
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

int rawInit() {
    // Create a hidden window to receive WM_INPUT
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RawInputWindow";

    if (!RegisterClass(&wc)) {
        printf("RegisterClass failed: %lu\n", GetLastError());
        return 1;
    }

    g_hwnd = CreateWindowEx(0, wc.lpszClassName, "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!g_hwnd) {
        printf("CreateWindowEx failed: %lu\n", GetLastError());
        return 1;
    }

    RAWINPUTDEVICE Rid[2];

    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x05; // Gamepad
    Rid[0].dwFlags = RIDEV_INPUTSINK; // receive input even if unfocused
    Rid[0].hwndTarget = g_hwnd;

    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage = 0x04; // Joystick
    Rid[1].dwFlags = RIDEV_INPUTSINK;
    Rid[1].hwndTarget = g_hwnd;

    if (RegisterRawInputDevices(Rid, 2, sizeof(RAWINPUTDEVICE)) == FALSE) {
        printf("Registration Failed: %lu\n", GetLastError());
        return 1;
    }

    printf("Registration Completed.\n");
    return 0;
}

int main() {
    if (rawInit() != 0) return 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
