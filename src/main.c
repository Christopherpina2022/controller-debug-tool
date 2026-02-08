#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <input.h>

/* This defines the space we allocate for the controller, also really useful
for offseting the spacing between controllers in RenderController. padding can also allow
us to add a cosmetic flair such as filling the border with a character or something. */ 
#define CONTROLLER_PANEL_WIDTH 30
#define CONTROLLER_PANEL_PADDING 2

#define CONTROLLER_STRIDE (CONTROLLER_PANEL_WIDTH + CONTROLLER_PANEL_PADDING)

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

void renderController(ConsoleScreen *screen, const GamepadState *state, int conIndex) {
    char tempBuffer[32];
    int xOffset = conIndex * CONTROLLER_STRIDE;

    if (!state || !state->connected) {
        toBuffer(screen, xOffset, 0, "No Controller.");
        return;
    }

    sprintf(tempBuffer, "Controller %d", conIndex);
    toBuffer(screen, xOffset, 0, tempBuffer);
    toBuffer(screen, xOffset, 1, "============");

    sprintf(tempBuffer, "D-UP: %-8s", (state->buttons & BTN_DPAD_UP) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 3, tempBuffer);

    sprintf(tempBuffer, "D-DOWN: %-8s", (state->buttons & BTN_DPAD_DOWN) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 4, tempBuffer);

    sprintf(tempBuffer, "D-LEFT: %-8s", (state->buttons & BTN_DPAD_LEFT) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 5, tempBuffer);

    sprintf(tempBuffer, "D-RIGHT: %-8s", (state->buttons & BTN_DPAD_RIGHT) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 6, tempBuffer);

    sprintf(tempBuffer, "LX: %+0.3f", state->axes[INPUT_AXIS_LEFT_X]);
    toBuffer(screen, xOffset, 8, tempBuffer);

    sprintf(tempBuffer, "LY: %+0.3f", state->axes[INPUT_AXIS_LEFT_Y]);
    toBuffer(screen, xOffset, 9, tempBuffer);

    sprintf(tempBuffer, "RX: %+0.3f", state->axes[INPUT_AXIS_RIGHT_X]);
    toBuffer(screen, xOffset, 11, tempBuffer);

    sprintf(tempBuffer, "RY: %+0.3f", state->axes[INPUT_AXIS_RIGHT_Y]);
    toBuffer(screen, xOffset, 12, tempBuffer);

    sprintf(tempBuffer, "LT/Z: %+0.3f", state->axes[INPUT_AXIS_LT]);
    toBuffer(screen, xOffset, 14, tempBuffer);

    sprintf(tempBuffer, "RT/RZ: %+0.3f", state->axes[INPUT_AXIS_RT]);
    toBuffer(screen, xOffset, 15, tempBuffer);

    sprintf(tempBuffer, "A: %-8s", (state->buttons & BTN_A) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 17, tempBuffer);

    sprintf(tempBuffer, "B: %-8s", (state->buttons & BTN_B) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 18, tempBuffer);

    sprintf(tempBuffer, "X: %-8s", (state->buttons & BTN_X) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 19, tempBuffer);

    sprintf(tempBuffer, "Y: %-8s", (state->buttons & BTN_Y) ? "Pressed" : "Released");
    toBuffer(screen, xOffset, 20, tempBuffer);
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
        input_update();

        for (int conIndex = 0; conIndex < MAX_CONTROLLERS; conIndex++) {
            int xOffset = conIndex * CONTROLLER_STRIDE;

            if (xOffset + CONTROLLER_PANEL_WIDTH >= screen.width)
                break;

            const GamepadState *pad = input_get_gamepad(conIndex);
            renderController(&screen, pad, conIndex);
        }
        flushBuffer(&screen);
        Sleep(16);

        const GamepadState *pad = input_get_gamepad(0);
    }
}