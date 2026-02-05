#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <input.h>

typedef enum {
    INPUT_BACKEND_XINPUT,
    INPUT_BACKEND_RAWINPUT
} InputBackend;

typedef struct {
    int height;
    int width;
    char *buffer;
    HANDLE console;
} ConsoleScreen;

void toBuffer (ConsoleScreen *screen, int x, int y, const char *string) {
    if (y < 0 || y >= screen->height) return;

    for (int i = 0; string[i]; i++) {
        int px = x + i;
        if (px < 0 || px >= screen->width) break;
        screen->buffer[y * screen->width + px] = string[i];
    }
}

void renderController(ConsoleScreen *screen, const GamepadState *state) {
    char tempBuffer[32];

    if (!state || !state->connected) {
        toBuffer(screen, 0, 0, "Controller is not connected.");
        return;
    }

    toBuffer(screen, 0, 0, "Controller");
    toBuffer(screen, 0, 1, "============");

    sprintf(tempBuffer, "D-UP: %-8s", (state->buttons & BTN_DPAD_UP) ? "Pressed" : "Released");
    toBuffer(screen, 0, 3, tempBuffer);

    sprintf(tempBuffer, "D-DOWN: %-8s", (state->buttons & BTN_DPAD_DOWN) ? "Pressed" : "Released");
    toBuffer(screen, 0, 4, tempBuffer);

    sprintf(tempBuffer, "D-LEFT: %-8s", (state->buttons & BTN_DPAD_LEFT) ? "Pressed" : "Released");
    toBuffer(screen, 0, 5, tempBuffer);

    sprintf(tempBuffer, "D-RIGHT: %-8s", (state->buttons & BTN_DPAD_RIGHT) ? "Pressed" : "Released");
    toBuffer(screen, 0, 6, tempBuffer);

    sprintf(tempBuffer, "LX: %+0.3f", state->axes[INPUT_AXIS_LEFT_X]);
    toBuffer(screen, 0, 8, tempBuffer);

    sprintf(tempBuffer, "LY: %+0.3f", state->axes[INPUT_AXIS_LEFT_Y]);
    toBuffer(screen, 0, 9, tempBuffer);

    sprintf(tempBuffer, "RX: %+0.3f", state->axes[INPUT_AXIS_RIGHT_X]);
    toBuffer(screen, 0, 11, tempBuffer);

    sprintf(tempBuffer, "RY: %+0.3f", state->axes[INPUT_AXIS_RIGHT_Y]);
    toBuffer(screen, 0, 12, tempBuffer);

    sprintf(tempBuffer, "LT/Z: %+0.3f", state->axes[INPUT_AXIS_LT]);
    toBuffer(screen, 0, 14, tempBuffer);

    sprintf(tempBuffer, "RT/RZ: %+0.3f", state->axes[INPUT_AXIS_RT]);
    toBuffer(screen, 0, 15, tempBuffer);
}

void flushBuffer(ConsoleScreen *screen) {
    DWORD written;
    COORD origin = {0,0};

    WriteConsoleOutputCharacterA(
        screen->console,
        screen->buffer,
        screen->width * screen->height,
        origin,
        &written
    );
}

void clearRegion(ConsoleScreen *screen) {
    memset(screen->buffer, ' ', screen->width * screen->height);
}

int main () {
    // Initialize console stuctures
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    ConsoleScreen screen;

    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // Load ConsoleScreen struct values
    screen.width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    screen.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    screen.console = hConsole;
    screen.buffer = malloc(screen.width * screen.height);

    COORD size = {
        (SHORT)screen.width,
        (SHORT)screen.height
    };
    
    SetConsoleScreenBufferSize(screen.console, size);
    system("cls");

    input_init();
    
    while (1) {
        clearRegion(&screen);

        // We are only grabbing data on the first controller we see for now
        input_update();
        const GamepadState *padState = input_get_gamepad(0);
        renderController(&screen, padState);
        flushBuffer(&screen);
        Sleep(16);

        const GamepadState *pad = input_get_gamepad(0);
    }
}