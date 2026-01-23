#include <xinput_Backend.h>
#include <input.h>
#include <math.h>
#include <windows.h>
#include <xinput.h>

// Defined by the Microsoft Xinput Documentation page
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 30
#define MAGNITUDE_MAX 32767
// User defined, range acceptable is from 0-65534
#define INPUT_DEADZONE 50.0f

static GamepadState controllers[MAX_CONTROLLERS];

float setDeadzone (XINPUT_STATE *state) {
    // Dead Zone variables (not useful until i get an actual controller instead of this fight stick)
    float LX = state->Gamepad.sThumbLX;
    float LY = state->Gamepad.sThumbLY;
    float normalizedLX = 0.0f;
    float normalizedLY = 0.0f;
    float magnitude = sqrt(LX * LX + LY * LY);

    // prevents a divide by zero error when the stick is idle
    if (magnitude > 0.0f) {
        normalizedLX = LX / magnitude;
        normalizedLY = LY / magnitude;
    }
}

void xinput_init() {
    memset(controllers, 0, sizeof(controllers));
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

static uint16_t mapButtons(WORD buttons) {
    uint16_t out = 0;

    if (buttons & XINPUT_GAMEPAD_A)              out |= BTN_A;
    if (buttons & XINPUT_GAMEPAD_B)              out |= BTN_B;
    if (buttons & XINPUT_GAMEPAD_X)              out |= BTN_X;
    if (buttons & XINPUT_GAMEPAD_Y)              out |= BTN_Y;

    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER)  out |= BTN_LB;
    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) out |= BTN_RB;

    if (buttons & XINPUT_GAMEPAD_BACK)           out |= BTN_BACK;
    if (buttons & XINPUT_GAMEPAD_START)          out |= BTN_START;

    if (buttons & XINPUT_GAMEPAD_LEFT_THUMB)     out |= BTN_LS;
    if (buttons & XINPUT_GAMEPAD_RIGHT_THUMB)    out |= BTN_RS;

    if (buttons & XINPUT_GAMEPAD_DPAD_UP)        out |= BTN_DPAD_UP;
    if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)      out |= BTN_DPAD_DOWN;
    if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)      out |= BTN_DPAD_LEFT;
    if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)     out |= BTN_DPAD_RIGHT;

    return out;
}

void xinput_update () {
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        XINPUT_STATE state;
        XINPUT_BATTERY_INFORMATION batteryInfo;
        GamepadState *padState = &controllers[i];
        ZeroMemory(&state, sizeof(state));

        DWORD batteryResult = XInputGetBatteryInformation(0, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo);
        DWORD result = XInputGetState(0, &state);

        // Will be set in the Xinput backend for now, but want to make it universal to both Xinput and Raw
        //float normalizedMagnitude = setDeadzone(&state);

        if (result == ERROR_SUCCESS){
            padState->connected = 1;

            padState->lx = state.Gamepad.sThumbLX;
            padState->ly = state.Gamepad.sThumbLY;
            padState->rx = state.Gamepad.sThumbRX;
            padState->ry = state.Gamepad.sThumbRY;

            padState->lt = state.Gamepad.bLeftTrigger;
            padState->rt = state.Gamepad.bRightTrigger;

            padState->buttons = mapButtons(state.Gamepad.wButtons);
        }
        else {
            controllers[i].connected - 0;
        }
    } 
}

const GamepadState *xinput_get_gamepad(int index) {
    if (index < 0 || index >= MAX_CONTROLLERS) {
        return NULL;
    }
    return &controllers[index];
}