#include <windows.h>
#include <xinput.h>
#include <stdio.h>
#include <math.h>

// Defined by the Microsoft Xinput Documentation page
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 30
#define MAGNITUDE_MAX 32767
// User defined, range acceptable is from 0-65534
#define INPUT_DEADZONE 0

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

char *batteryLevel(BYTE batLevel) {
    switch (batLevel)
    {
    case BATTERY_LEVEL_EMPTY:   return "Empty";
    case BATTERY_LEVEL_LOW:     return "Low";
    case BATTERY_LEVEL_MEDIUM:  return "Medium";
    case BATTERY_LEVEL_FULL:    return "Full";
    default:                    return "Unknown";
    }
}

char *batteryType(BYTE batType) {
    switch (batType)
    {
    case BATTERY_TYPE_WIRED:    return "Wired";
    case BATTERY_TYPE_ALKALINE: return "Alkaline";
    case BATTERY_TYPE_NIMH:     return "Nickel Metal Hydride";
    case BATTERY_TYPE_UNKNOWN:  return "Unknown";
    default:
    }
}

void renderController(ConsoleScreen *screen, XINPUT_STATE *state) {
    XINPUT_BATTERY_INFORMATION batteryInfo;
    WORD buttons = state->Gamepad.wButtons;
    char tempBuffer[32];

    DWORD batteryResult = XInputGetBatteryInformation(0, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo);

    toBuffer(screen, 0, 0, "Controller 0");
    toBuffer(screen, 0, 1, "============");

    sprintf(tempBuffer, "Battery: %s, Battery Type: %s", batteryLevel(batteryInfo.BatteryLevel), batteryType(batteryInfo.BatteryType));
    toBuffer(screen, 0, 2, tempBuffer);

    sprintf(tempBuffer, "A: %-8s", (XINPUT_GAMEPAD_A & buttons) ? "Pressed" : "Released");
    toBuffer(screen, 0, 4, tempBuffer);

    sprintf(tempBuffer, "B: %-8s", (XINPUT_GAMEPAD_B & buttons) ? "Pressed" : "Released");
    toBuffer(screen, 0, 5, tempBuffer);

    sprintf(tempBuffer, "X: %-8s", (XINPUT_GAMEPAD_X & buttons) ? "Pressed" : "Released");
    toBuffer(screen, 0, 6, tempBuffer);

    sprintf(tempBuffer, "Y: %-8s", (XINPUT_GAMEPAD_Y & buttons) ? "Pressed" : "Released");
    toBuffer(screen, 0, 7, tempBuffer);

    sprintf(tempBuffer, "LX: %6d", (state->Gamepad.sThumbLX));
    toBuffer(screen, 0, 9, tempBuffer);

    sprintf(tempBuffer, "LY: %d", (state->Gamepad.sThumbLY));
    toBuffer(screen, 0, 10, tempBuffer);
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

void setDeadzone (float *magnitude, float *normalizedMagnitude) {
    if (*magnitude > INPUT_DEADZONE) {
        if (*magnitude > MAGNITUDE_MAX) *magnitude = MAGNITUDE_MAX;
        *magnitude -= INPUT_DEADZONE;
        *normalizedMagnitude = *magnitude / (MAGNITUDE_MAX - INPUT_DEADZONE);
    }
    else {
        *magnitude = 0.0;
        *normalizedMagnitude = 0.0;
    }
}

int main () {
    // Initialize XInput structures
    XINPUT_STATE state;
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

    // Dead Zone variables (not useful until i get an actual controller instead of this fight stick)
    float LX = state.Gamepad.sThumbLX;
    float LY = state.Gamepad.sThumbLY;
    float magnitude = sqrt(LX * LY + LY * LY);
    float normalizedLX = LX / magnitude;
    float normalizedLY = LY / magnitude;
    float normalizedMagnitude = 0;

    setDeadzone(&magnitude, &normalizedMagnitude);
    SetConsoleScreenBufferSize(screen.console, size);
    system("cls");
    
    while (1) {
        clearRegion(&screen);

        // Checks if our controller is plugged in by clearing the memory of it every frame
        ZeroMemory(&state, sizeof(state));
        DWORD result = XInputGetState(0, &state);
        
        if (result == ERROR_SUCCESS)
        {
            renderController(&screen, &state);
        }
        else {
            clearRegion(&screen);
            toBuffer(&screen, 0, 0, "controller is not plugged in.");
        }

        flushBuffer(&screen);
        Sleep(16);
    }
}