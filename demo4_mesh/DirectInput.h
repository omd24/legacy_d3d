#pragma once

// Wraps initialization of immediate mode Direct Input, and provides 
// information for querying the state of the keyboard and mouse.

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

struct DirectInput {
    IDirectInput8 *         dinput;

    IDirectInputDevice8 *   keyboard;
    char                    keyboard_state[256];

    IDirectInputDevice8 *   mouse;
    DIMOUSESTATE2           mouse_state;
};
void
DirectInput_Init (
    DirectInput * dinp, DWORD keyboard_flags, DWORD mouse_flags,
    HINSTANCE inst, HWND wnd
);
void
DirectInput_Deinit (DirectInput * dinp);
void
DirectInput_Poll (DirectInput * dinp);
bool
DirectInput_KeyDown (DirectInput * dinp, char key);
bool
DirectInput_MouseButtonDown (DirectInput * dinp, int button);
float
DirectInput_MouseDx (DirectInput * dinp);
float
DirectInput_MouseDy (DirectInput * dinp);
float
DirectInput_MouseDz (DirectInput * dinp);
