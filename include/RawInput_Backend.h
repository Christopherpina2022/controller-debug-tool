#pragma once
#include <input.h>

#define SONY_VID 0x054C
#define NINTENDO_VID 0x057E

/* PIDs may not actually be used since the VID may be good enough
to map buttons and axes. We aren't considering extra features anyway.*/

#define SONY_PID_DS3 0x0268
#define SONY_PID_DS4_V1 0x05C4
#define SONY_PID_DS4_V2 0x09CC
#define SONY_PID_PS5 0x0CE6
#define SONY_PID_PCC 0x0CDA

#define NINTENDO_PID_SPC 0x2009
#define NINTENDO_PID_SJC_1 0x2006
#define NINTENDO_PID_SJC_2 0x2007
#define NINTENDO_PID_WII 0x0306

void rawInit(void);
void rawUpdate(void);
const GamepadState *rawinput_get_gamepad(int index);