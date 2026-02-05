#pragma once
#include <windows.h>
#include <input.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <stdint.h>

#define MAX_USAGES 128
#define HID_MAP_UNUSED -1
#define DEADZONE 0.15f

// Creates a device record that we call once per device. caps are short for capabilities.
typedef struct {
    // input mapping
    int buttonMap[MAX_USAGES];
    int dpadMap[MAX_USAGES];
    int axisMap[MAX_USAGES];

    // identity
    uint16_t vendorID;
    uint16_t productID;

    // HID info that is useful for us
    HANDLE device;
    PHIDP_PREPARSED_DATA preparsed;
    HIDP_CAPS caps;
    HIDP_BUTTON_CAPS *buttonCaps;
    USHORT buttonCapCount;
    HIDP_VALUE_CAPS *valueCaps;
    USHORT valueCapCount;

    // Output
    GamepadState *state;
} HidRecord;

void rawInit(void);
void rawUpdate(void);
const GamepadState *rawinput_get_gamepad(int index);