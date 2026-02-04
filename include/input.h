#pragma once
#include <stdint.h>

#define MAX_CONTROLLERS 4

typedef enum {
    GAMEPAD_BTN_DPAD_UP,
    GAMEPAD_BTN_DPAD_DOWN,
    GAMEPAD_BTN_DPAD_LEFT,
    GAMEPAD_BTN_DPAD_RIGHT,

    GAMEPAD_BTN_COUNT
} GamepadButtonIndex;

// right number is the assignment for when you map the buttons
typedef enum {
    BTN_A       = 1 << 0,
    BTN_B       = 1 << 1,
    BTN_X       = 1 << 2,
    BTN_Y       = 1 << 3,

    BTN_LB      = 1 << 4,
    BTN_RB      = 1 << 5,

    BTN_BACK    = 1 << 6,
    BTN_START   = 1 << 7,

    BTN_LS      = 1 << 8,
    BTN_RS      = 1 << 9,

    BTN_DPAD_UP    = 1 << GAMEPAD_BTN_DPAD_UP,
    BTN_DPAD_DOWN  = 1 << GAMEPAD_BTN_DPAD_DOWN,
    BTN_DPAD_LEFT  = 1 << GAMEPAD_BTN_DPAD_LEFT,
    BTN_DPAD_RIGHT = 1 << GAMEPAD_BTN_DPAD_RIGHT
} GamepadButtons;

typedef enum {
    AXIS_LX = 0, // HID 0x30
    AXIS_LY = 1, // HID 0x31
    AXIS_LZ = 2, // HID 0x32 (often left trigger or Z)
    AXIS_RX = 3, // HID 0x33
    AXIS_RY = 4, // HID 0x34
    AXIS_RZ = 5, // HID 0x35 (often right trigger)

    AXIS_COUNT
} GamepadAxis;

typedef struct {
    int connected;
    float axes[AXIS_COUNT];
    uint16_t buttons; 
} GamepadState;

void input_init(void);
void input_update(void);
const GamepadState *input_get_gamepad(int index);