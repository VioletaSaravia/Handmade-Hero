#include "handmade.h"

enum ButtonState : u8 { Released, JustReleased, Pressed, JustPressed };

enum GamepadButton : u8 {
    Up,
    Down,
    Left,
    Right,
    A,
    B,
    X,
    Y,
    ShL,
    ShR,
    Start,
    Back,
    ThumbL,
    ThumbR,
    COUNT
};

struct ControllerState {
    bool connected, notAnalog;

    ButtonState buttons[GamepadButton::COUNT];
    f32         triggerLStart, triggerLEnd, triggerRStart, triggerREnd;

    v2  analogLStart, analogLEnd, analogRStart, analogREnd;
    f32 minX, minY, maxX, maxY;  // TODO(violeta): Deadzones
};

#define KEY_COUNT 256

// NOTE(violeta): These are win32 key codes. They're just here to make the code easy on windows.
// Might be a pain to adapt to other platforms, but they should work.
enum Key : u8 {
    Ret      = 0x0D,
    Shift    = 0x10,
    Ctrl     = 0x11,
    Esc      = 0x1B,
    Space    = 0x20,
    KeyLeft  = 0x25,
    KeyUp    = 0x26,
    KeyRight = 0x27,
    KeyDown  = 0x28,
};

struct KeyboardState {
    ButtonState keys[KEY_COUNT];
};

#define CONTROLLER_COUNT 4

struct InputBuffer {
    ControllerState controllers[CONTROLLER_COUNT];
    KeyboardState   keyboard;
};

struct SoundBuffer {
    u8 *sampleOut;
    u32 byteCount;
    u32 sampleCount;
    u32 samplesPerSec;
};

internal void OutputSound(SoundBuffer *buffer) {
    // for (u32 i = 0; i < buffer->byteCount;) {
    //     i16 sample             = 0;
    //     buffer->sampleOut[i++] = (u8)sample;  // Values are little-endian.
    //     buffer->sampleOut[i++] = (u8)(sample >> 8);
    // }
}

struct ScreenBuffer {
    u8 *Memory;
    i32 Width, Height;
    i32 BytesPerPixel;
};

struct Memory {
    u64   permStoreSize;
    void *permStore;
    u64   scratchStoreSize;
    void *scratchStore;
};

struct GameState {
    f64 delta;
    v2i pos[2];
};

internal void Render(ScreenBuffer *buffer, const GameState *state) {
    i32 stride = buffer->Width * buffer->BytesPerPixel;
    u8 *row    = (u8 *)buffer->Memory;
    for (i32 y = 0; y < buffer->Height; y++) {
        u32 *pixel = (u32 *)row;
        for (i32 x = 0; x < buffer->Width; x++) {
            u8 red = 0, green = 0, blue = 0;

            // f32 dist;
            // for (i32 i = 0; i < 1; i++) {
            //     dist = sqrtf(powf(f32(state->pos[i].x - x), 2) +
            //                  powf(f32(state->pos[i].y / 2 - y), 2));

            //     red += dist < 50 ? 255 : 0;
            // }

            // Pixel: BB GG RR -- (4 bytes)
            //        0  1  2  3
            *pixel++ = u32(blue | (green << 8) | (red << 16));
        }

        row += stride;
    }
}

internal void InitState(Memory *memory) {
    GameState *state = (GameState *)memory->permStore;

    *state = {.pos = {{400, 300}}};
}

internal void UpdateAndRender(Memory *memory, InputBuffer *input, ScreenBuffer *screen,
                              SoundBuffer *sound) {
    Assert(sizeof(memory->permStore) >= sizeof(GameState));
    GameState *state = (GameState *)memory->permStore;

    if (input->keyboard.keys['D'] >= ::Pressed) {
        state->pos[0].x += 10;
    }
    if (input->keyboard.keys['A'] >= ::Pressed) {
        state->pos[0].x -= 10;
    }
    if (input->keyboard.keys['W'] >= ::Pressed) {
        state->pos[0].y -= 10;
    }
    if (input->keyboard.keys['S'] >= ::Pressed) {
        state->pos[0].y += 10;
    }

    OutputSound(sound);
    Render(screen, state);
}