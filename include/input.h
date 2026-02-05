#pragma once
#include <stdint.h>

#define MAX_CONTROLLERS 4
#define MAP_UNUSED -1

typedef enum {
    INPUT_AXIS_LEFT_X,
    INPUT_AXIS_LEFT_Y,
    INPUT_AXIS_RIGHT_X,
    INPUT_AXIS_RIGHT_Y,
    INPUT_AXIS_LT,
    INPUT_AXIS_RT,
    INPUT_AXIS_COUNT
} InputAxis;

typedef enum {
    INPUT_BTN_A,
    INPUT_BTN_B,
    INPUT_BTN_X,
    INPUT_BTN_Y,
    INPUT_BTN_BACK,
    INPUT_BTN_START,
    INPUT_BTN_LB,
    INPUT_BTN_RB,
    INPUT_BTN_LS,
    INPUT_BTN_RS,
    INPUT_BTN_COUNT
} InputButton;

typedef enum {
    INPUT_DPAD_UP,
    INPUT_DPAD_DOWN,
    INPUT_DPAD_LEFT,
    INPUT_DPAD_RIGHT
} InputDpad;

// right number is the assignment for when you map the buttons
typedef enum {
    BTN_A       = 1 << INPUT_BTN_A,
    BTN_B       = 1 << INPUT_BTN_B,
    BTN_X       = 1 << INPUT_BTN_X,
    BTN_Y       = 1 << INPUT_BTN_Y,

    BTN_LB      = 1 << INPUT_BTN_LB,
    BTN_RB      = 1 << INPUT_BTN_RB,

    BTN_BACK    = 1 << INPUT_BTN_BACK,
    BTN_START   = 1 << INPUT_BTN_START,

    BTN_LS      = 1 << INPUT_BTN_LS,
    BTN_RS      = 1 << INPUT_BTN_RS,

    BTN_DPAD_UP    = 1 << INPUT_DPAD_UP,
    BTN_DPAD_DOWN  = 1 << INPUT_DPAD_DOWN,
    BTN_DPAD_LEFT  = 1 << INPUT_DPAD_LEFT,
    BTN_DPAD_RIGHT = 1 << INPUT_DPAD_RIGHT
} GamepadBitmask;

typedef struct {
    int connected;
    float axes[INPUT_AXIS_COUNT];
    uint32_t buttons; 
} GamepadState;

void input_init(void);
void input_update(void);
const GamepadState *input_get_gamepad(int index);