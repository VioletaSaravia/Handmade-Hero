#include "win32.h"

static Keybind Keys[ACTION_COUNT][MAX_KEYBINDS] = {
    [ACTION_UP]     = {{INPUT_Keyboard, 'W'}, {INPUT_Gamepad1, PAD_Up}},
    [ACTION_DOWN]   = {0},
    [ACTION_LEFT]   = {0},
    [ACTION_RIGHT]  = {0},
    [ACTION_ACCEPT] = {0},
    [ACTION_CANCEL] = {0},
};

void ProcessKeyboard(ButtonState *keys, bool *running) {
    for (i32 i = 0; i < KEY_COUNT; i++) {
        if (keys[i] == JustReleased) keys[i] = Released;
        if (keys[i] == JustPressed) keys[i] = Pressed;
    }

    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, 1)) {
        if (msg.message == WM_QUIT) *running = 0;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void ProcessGamepads(GamepadState *gamepads) {
    for (i32 i = 0; i < GAMEPAD_MAX; i++) {
        GamepadState new       = {0};
        const GamepadState old = gamepads[i];

        XINPUT_STATE state = {0};
        if (XInputGetState(i, &state) != ERROR_SUCCESS) {
            if (old.connected) new.connected = 0;
            continue;
        }

        if (!old.connected) new.connected = 1;

        XINPUT_GAMEPAD *gamepad = &state.Gamepad;

        for (i32 j = 0; j < PAD_ButtonCount; j++) {
            switch (old.buttons[j]) {
            case Pressed:
            case JustPressed:
                // FIXME ???
                new.buttons[j] =
                    XINPUT_GAMEPAD_DPAD_UP | gamepad->wButtons ? Pressed : JustReleased;
                break;

            case Released:
            case JustReleased:
                new.buttons[j] =
                    XINPUT_GAMEPAD_DPAD_UP | gamepad->wButtons ? Pressed : JustReleased;
                break;
            }
        }

        new.lStart = old.lEnd;
        new.rStart = old.rEnd;

        f32 stickLX = (f32)(gamepad->sThumbLX) / (gamepad->sThumbLX < 0 ? -32768.0f : 32767.0f);
        f32 stickLY = (f32)(gamepad->sThumbLY) / (gamepad->sThumbLY < 0 ? -32768.0f : 32767.0f);
        f32 stickRX = (f32)(gamepad->sThumbRX) / (gamepad->sThumbRX < 0 ? -32768.0f : 32767.0f);
        f32 stickRY = (f32)(gamepad->sThumbRY) / (gamepad->sThumbRY < 0 ? -32768.0f : 32767.0f);

        new.lEnd = (v2){stickLX, stickLY};
        new.rEnd = (v2){stickRX, stickRY};

        new.trigLStart = old.trigLEnd;
        new.trigRStart = old.trigREnd;
        new.trigLEnd   = (f32)(gamepad->bLeftTrigger) / 255.0f;
        new.trigREnd   = (f32)(gamepad->bRightTrigger) / 255.0f;

        gamepads[i] = new;
    }
}

void ProcessMouse(MouseState *mouse) {
    // NOTE: La rueda del mouse se maneja en MainWindowCallback
    mouse->pos = Win32GetMouse();

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        mouse->left = mouse->left >= Pressed ? Pressed : JustPressed;
    else
        mouse->left = mouse->left <= Released ? Released : JustReleased;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        mouse->right = mouse->right >= Pressed ? Pressed : JustPressed;
    else
        mouse->right = mouse->right <= Released ? Released : JustReleased;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
        mouse->middle = mouse->middle >= Pressed ? Pressed : JustPressed;
    else
        mouse->middle = mouse->middle <= Released ? Released : JustReleased;
}