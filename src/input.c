#include <input.h>
#include <XInput_Backend.h>
#include <RawInput_Backend.h>

typedef enum {
    INPUT_BACKEND_RAW,
    INPUT_BACKEND_XINPUT
} InputBackend;

static InputBackend activeBackend = INPUT_BACKEND_RAW;

void input_init() {
    xinput_init();
    rawInit();
}

void input_update() {
    rawUpdate();
    xinput_update();

    int hasXInput = 0;
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        const GamepadState *g = xinput_get_gamepad(i);
        if (g && g->connected) {
            hasXInput = 1;
            break;
        }
    }

    activeBackend = hasXInput ? INPUT_BACKEND_XINPUT : INPUT_BACKEND_RAW;
}


const GamepadState *input_get_gamepad(int index) {
    switch (activeBackend)
    {
        case INPUT_BACKEND_RAW:
            return rawinput_get_gamepad(index);
        case INPUT_BACKEND_XINPUT:
            return xinput_get_gamepad(index);
    }
    return NULL;
}