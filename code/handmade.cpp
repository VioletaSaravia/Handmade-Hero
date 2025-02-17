#include "handmade.h"

enum ButtonState : u8 { Released, JustReleased, Pressed, JustPressed };

enum GamepadButton : u8 { Up, Down, Left, Right, A, B, X, Y, ShL, ShR, Start, Back, COUNT };

struct ControllerState {
    bool connected, notAnalog;

    ButtonState buttons[GamepadButton::COUNT];
    f32         triggerLStart, triggerLEnd, triggerRStart, triggerREnd;

    v2  analogLStart, analogLEnd, analogRStart, analogREnd;
    f32 minX, minY, maxX, maxY;
};

#define KEY_COUNT 256

// NOTE(violeta): These are win32 key codes. They make the code easy on windows, but they might
// be a pain in other platforms, but they should work.
enum Key : u8 {
    Shift    = 0x10,
    Ctrl     = 0x11,
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
    for (u32 i = 0; i < buffer->byteCount;) {
        i16 sample             = 0;
        buffer->sampleOut[i++] = (u8)sample;  // Values are little-endian.
        buffer->sampleOut[i++] = (u8)(sample >> 8);
    }
}

struct ScreenBuffer {
    u8 *Memory;
    i32 Width, Height;
    i32 BytesPerPixel;
};

struct Memory {
    bool  isInitialized;
    u64   permStoreSize;
    void *permStore;
    u64   scratchStoreSize;
    void *scratchStore;
};

struct GameState {};

internal void RenderWeirdGradient(ScreenBuffer *buffer, int xOffset, int yOffset) {
    i32 stride = buffer->Width * buffer->BytesPerPixel;
    u8 *row    = (u8 *)buffer->Memory;
    for (i32 y = 0; y < buffer->Height; y++) {
        u32 *pixel = (u32 *)row;
        for (i32 x = 0; x < buffer->Width; x++) {
            /*
            Pixel: BB GG RR -- (4 bytes)
                   0  1  2  3
            */
            u8 blue  = u8(x + xOffset);
            u8 green = u8(y + yOffset);
            u8 red   = 0;

            *pixel++ = u32(blue | (green << 8) | (red << 16));
        }

        row += stride;
    }
}

internal void UpdateAndRender(Memory *memory, InputBuffer *input, ScreenBuffer *screen,
                              SoundBuffer *sound) {
    Assert(sizeof(memory->permStore) >= sizeof(GameState));
    GameState *state = (GameState *)memory->permStore;

    OutputSound(sound);
    RenderWeirdGradient(screen, 0, 0);
}