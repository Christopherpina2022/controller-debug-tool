#pragma once
#include <windows.h>
#include <input.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <stdint.h>

#define MAX_USAGES 128

// Creates a device record that we call once per device. caps are short for capabilities.
typedef struct {
    // input mapping
    int buttonMap[MAX_USAGES];
    int axisMap[MAX_USAGES];

    // HID info that is useful for us
    uint16_t vendorID;
    uint16_t productID;
    HANDLE device;
    PHIDP_PREPARSED_DATA preparsed;
    HIDP_CAPS caps;
    HIDP_BUTTON_CAPS *buttonCaps;
    USHORT buttonCapCount;
    HIDP_VALUE_CAPS *valueCaps;
    USHORT valueCapCount;


} HidRecord;

void rawInit(void);
void rawUpdate(void);
const GamepadState *rawinput_get_gamepad(int index);