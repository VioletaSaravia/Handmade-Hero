#pragma once

#include "common.h"

struct Shader {
    u32 ID;

    Shader(const char* vertFile, const char* fragFile);

    void SetUniform(const char* name, i32 value);
    void SetUniform(const char* name, f32 value);
    void SetUniform(const char* name, v2 value);
};

//global Shader DefaultShader;

struct Texture {
    u32 id;
    i32 width, height, nChannels;

    Texture(const char* path);
};

struct Object {
    u32     VAO;
    Shader  shader;
    Texture texture;

    Object(Texture texture, Shader shader, const f32* vertices, u32 vSize, const u32* indices,
           u32 iSize);

    void Draw();
};

#ifdef HANDMADE_INTERNAL
void* PlatformReadEntireFile(char* filename);
void  PlatformFreeFileMemory(void* memory);
bool  PlatformWriteEntireFile(char* filename, u32 size, void* memory);
#endif

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

// TODO(violeta): These are win32 key codes. They're just here to make the code easy on windows.
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

#define KEY_COUNT 256

struct KeyboardState {
    ButtonState keys[KEY_COUNT];
};

#define CONTROLLER_COUNT 4

struct InputBuffer {
    ControllerState controllers[CONTROLLER_COUNT];
    KeyboardState   keyboard;
};

struct Memory {
    u64   permStoreSize;
    void* permStore;
    u64   scratchStoreSize;
    void* scratchStore;
};

struct SoundBuffer {
    u32 sampleRate;
    u32 bitRate;
};

struct ScreenBuffer {
    u8* Memory;
    i32 Width, Height;
    i32 BytesPerPixel;
};

#define GAME_UPDATE(name)                                                          \
    void name(f64 delta, Memory* memory, InputBuffer* input, ScreenBuffer* screen, \
              SoundBuffer* sound)
typedef GAME_UPDATE(GameUpdate);
extern "C" GAME_UPDATE(GameUpdateStub) {}

#define GAME_INIT(name) void name(Memory* memory)
typedef GAME_INIT(GameInit);
extern "C" GAME_INIT(GameInitStub) {}

global v2 WindowSize = {800, 600};
