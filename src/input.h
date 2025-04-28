#pragma once
#include "engine.h"

typedef enum { Released, JustReleased, Pressed, JustPressed, BUTTONSTATE_COUNT } ButtonState;

typedef struct {
    bool        connected, notAnalog;
    ButtonState buttons[16];
    f32         trigLStart, trigLEnd, trigRStart, trigREnd;
    v2          lStart, lEnd, rStart, rEnd;
    f32         minX, minY, maxX, maxY;
} GamepadState;

typedef struct {
    i16                  wheel;
    v2                   pos;
    SDL_MouseButtonFlags cur, prev;
} MouseState;

typedef enum {
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_ACCEPT,
    ACTION_CANCEL,
    ACTION_COUNT,
} Action;

typedef struct {
    SDL_Keymod  mods;
    SDL_Keycode key;
} Keybind;

typedef enum {
    NONE   = SDL_KMOD_NONE,
    LSHIFT = SDL_KMOD_LSHIFT,
    RSHIFT = SDL_KMOD_RSHIFT,
    LEVEL5 = SDL_KMOD_LEVEL5,
    LCTRL  = SDL_KMOD_LCTRL,
    RCTRL  = SDL_KMOD_RCTRL,
    LALT   = SDL_KMOD_LALT,
    RALT   = SDL_KMOD_RALT,
    LGUI   = SDL_KMOD_LGUI,
    RGUI   = SDL_KMOD_RGUI,
    NUM    = SDL_KMOD_NUM,
    CAPS   = SDL_KMOD_CAPS,
    MODE   = SDL_KMOD_MODE,
    SCROLL = SDL_KMOD_SCROLL,
    CTRL   = SDL_KMOD_CTRL,
    SHIFT  = SDL_KMOD_SHIFT,
    ALT    = SDL_KMOD_ALT,
    GUI    = SDL_KMOD_GUI,
} KeyMod;

#define GAMEPAD_MAX XUSER_MAX_COUNT

struct InputCtx {
    GamepadState gamepads[4];
    ButtonState  keys[256];
    u16          mods[BUTTONSTATE_COUNT];
    MouseState   mouse;
};