#include "DirectInput.h"
#include "Common.h"

void
DirectInput_Init (
    DirectInput * dinp, DWORD keyboard_flags, DWORD mouse_flags,
    HINSTANCE inst, HWND wnd
) {
    memset(dinp->keyboard_state, 0, sizeof(dinp->keyboard_state));
    memset(&dinp->mouse_state, 0, sizeof(dinp->mouse_state));

    DirectInput8Create(inst, DIRECTINPUT_VERSION,
        IID_IDirectInput8, (void**)&dinp->dinput, 0);

    dinp->dinput->CreateDevice(GUID_SysKeyboard, &dinp->keyboard, 0);
    dinp->keyboard->SetDataFormat(&c_dfDIKeyboard);
    dinp->keyboard->SetCooperativeLevel(wnd, keyboard_flags);
    dinp->keyboard->Acquire();

    dinp->dinput->CreateDevice(GUID_SysMouse, &dinp->mouse, 0);
    dinp->mouse->SetDataFormat(&c_dfDIMouse2);
    dinp->mouse->SetCooperativeLevel(wnd, mouse_flags);
    dinp->mouse->Acquire();
}
void
DirectInput_Deinit (DirectInput * dinp) {
    dinp->dinput->Release();
    dinp->keyboard->Unacquire();
    dinp->mouse->Unacquire();
    dinp->keyboard->Release();
    dinp->mouse->Release();
}
void
DirectInput_Poll (DirectInput * dinp) {
    // Poll keyboard.
    HRESULT hr = dinp->keyboard->GetDeviceState(sizeof(dinp->keyboard_state), (void**)&dinp->keyboard_state);
    if (FAILED(hr)) {
        // Keyboard lost, zero out keyboard data structure.
        ZeroMemory(dinp->keyboard_state, sizeof(dinp->keyboard_state));

         // Try to acquire for next time we poll.
        hr = dinp->keyboard->Acquire();
    }

    // Poll mouse.
    hr = dinp->mouse->GetDeviceState(sizeof(DIMOUSESTATE2), (void**)&dinp->mouse_state);
    if (FAILED(hr)) {
        // Mouse lost, zero out mouse data structure.
        ZeroMemory(&dinp->mouse_state, sizeof(dinp->mouse_state));

        // Try to acquire for next time we poll.
        hr = dinp->mouse->Acquire();
    }
}
bool
DirectInput_KeyDown (DirectInput * dinp, char key) {
    return (dinp->keyboard_state[key] & 0x80) != 0;
}
bool
DirectInput_MouseButtonDown (DirectInput * dinp, int button) {
    return (dinp->mouse_state.rgbButtons[button] & 0x80) != 0;
}
float
DirectInput_MouseDx (DirectInput * dinp) {
    return (float)dinp->mouse_state.lX;
}
float
DirectInput_MouseDy (DirectInput * dinp) {
    return (float)dinp->mouse_state.lY;
}
float
DirectInput_MouseDz (DirectInput * dinp) {
    return (float)dinp->mouse_state.lZ;
}
