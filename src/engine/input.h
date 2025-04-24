#include "common.h"

typedef enum {
    PAD_Up,
    PAD_Down,
    PAD_Left,
    PAD_Right,
    PAD_A,
    PAD_B,
    PAD_X,
    PAD_Y,
    PAD_ShL,
    PAD_ShR,
    PAD_Start,
    PAD_Back,
    PAD_ThumbL,
    PAD_ThumbR,
    PAD_ButtonCount,
} GamepadButton;

typedef enum { JustReleased, Released, Pressed, JustPressed } ButtonState;

typedef struct {
    bool        connected, notAnalog;
    ButtonState buttons[PAD_ButtonCount];
    f32         trigLStart, trigLEnd, trigRStart, trigREnd;
    v2          lStart, lEnd, rStart, rEnd;
    f32         minX, minY, maxX, maxY;
} GamepadState;

typedef enum {
    KEY_Ret      = 0x0D,
    KEY_Shift    = 0x10,
    KEY_Ctrl     = 0x11,
    KEY_Esc      = 0x1B,
    KEY_Space    = 0x20,
    KEY_KeyLeft  = 0x25,
    KEY_KeyUp    = 0x26,
    KEY_KeyRight = 0x27,
    KEY_KeyDown  = 0x28,
    KEY_F1       = 0x70,
    KEY_F2       = 0x71,
    KEY_F3       = 0x72,
    KEY_F4       = 0x73,
    KEY_F5       = 0x74,
    KEY_F6       = 0x75,
    KEY_F7       = 0x76,
    KEY_F8       = 0x77,
    KEY_F9       = 0x78,
    KEY_F10      = 0x79,
    KEY_F11      = 0x7A,
    KEY_F12      = 0x7B,
    KEY_COUNT    = 0xFF, // 255?
} Key;

typedef struct {
    ButtonState left, right, middle;
    i16         wheel;
    v2          pos;
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

typedef enum {
    INPUT_Keyboard,
    INPUT_Mouse,
    INPUT_Gamepad1,
    INPUT_Gamepad2,
    INPUT_Gamepad3,
    INPUT_Gamepad4 // etc.
} InputSource;

#define MAX_MODS 4

typedef struct {
    InputSource source;
    u32         key;
    u32         mods[MAX_MODS];
} Keybind;

#define MAX_KEYBINDS 3

internal void ProcessKeyboard(ButtonState *keys, bool *running);
internal void ProcessGamepads(GamepadState *gamepads);
internal void ProcessMouse(MouseState *mouse);