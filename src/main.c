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

    sprintf(tempBuffer, "A: %-8s", (state->buttons & BTN_A) ? "Pressed" : "Released");
    toBuffer(screen, 0, 3, tempBuffer);

    sprintf(tempBuffer, "B: %-8s", (state->buttons & BTN_B) ? "Pressed" : "Released");
    toBuffer(screen, 0, 4, tempBuffer);

    sprintf(tempBuffer, "X: %-8s", (state->buttons & BTN_X) ? "Pressed" : "Released");
    toBuffer(screen, 0, 5, tempBuffer);

    sprintf(tempBuffer, "Y: %-8s", (state->buttons & BTN_Y) ? "Pressed" : "Released");
    toBuffer(screen, 0, 6, tempBuffer);

    sprintf(tempBuffer, "LX: %6d", (state->lx));
    toBuffer(screen, 0, 8, tempBuffer);

    sprintf(tempBuffer, "LY: %d", (state->ly));
    toBuffer(screen, 0, 9, tempBuffer);
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
    }
}