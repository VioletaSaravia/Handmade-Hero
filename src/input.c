#include "input.h"

InputCtx InitInput() {
    InputCtx result = {0};

    result.keyState = SDL_GetKeyboardState(0),
    result.gamepads = SDL_GetGamepads(&result.gamepadCount);
    for (i32 i = 0; i < result.gamepadCount; i++) {
        if (!SDL_OpenGamepad(result.gamepads[i]))
            LOG_ERROR("Couldn't open gamepad: ", SDL_GetError());
    }

    return result;
}

void UpdateInput(InputCtx *input) {
    memcpy(input->keyStatePrev, input->keyState, sizeof(bool) * SDL_SCANCODE_COUNT);

    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT: Window()->quit = true; break;
        default: break;
        }
    }

    input->gamepads = SDL_GetGamepads(&input->gamepadCount);
    for (i32 i = 0; i < input->gamepadCount; i++) {
        input->gamepadButtons[i].btnPrev = input->gamepadButtons[i].btnCur;
        input->gamepadButtons[i].btnCur  = 0;
        const char *mapping = SDL_GetGamepadMapping(SDL_GetGamepadFromID(input->gamepads[i]));
        printf("Mapping: %s\n", mapping);
        for (i32 j = 0; j < SDL_GAMEPAD_BUTTON_COUNT; j++) {
            if (SDL_GetGamepadButton(SDL_GetGamepadFromID(input->gamepads[i]), j))
                input->gamepadButtons[i].btnCur |= 1 << j;
        }
    }

    input->mousePrev    = input->mouseCur;
    input->mousePosPrev = input->mousePos;
    input->mouseCur     = SDL_GetMouseState(&input->mousePos.x, &input->mousePos.y);
}

BtnState GetPadButton(u32 pad, GamepadButton button) {
    if (pad >= Input()->gamepadCount) {
        LOG_WARNING("Pad not connected: %u", pad);
        return 0;
    }

    bool isCur  = Input()->gamepadButtons[pad].btnCur & (1 << button);
    bool isPrev = Input()->gamepadButtons[pad].btnPrev & (1 << button);

    if (isCur && isPrev) return Pressed;
    if (!isCur && isPrev) return JustReleased;
    if (isCur && !isPrev) return JustPressed;
    return Released;
}

BtnState GetKey(KeyboardKey code) {
    if (code >= KEY_COUNT) {
        LOG_WARNING("Invalid scancode: %u", code);
        return 0;
    }

    bool isCur  = Input()->keyState[code];
    bool isPrev = Input()->keyStatePrev[code];

    if (isCur && isPrev) return Pressed;
    if (!isCur && isPrev) return JustReleased;
    if (isCur && !isPrev) return JustPressed;
    return Released;
}

BtnState GetMouseButton(MouseButton button) {
    bool isCur  = Input()->mouseCur & button;
    bool isPrev = Input()->mousePrev & button;

    if (isCur && isPrev) return Pressed;
    if (!isCur && isPrev) return JustReleased;
    if (isCur && !isPrev) return JustPressed;
    return Released;
}