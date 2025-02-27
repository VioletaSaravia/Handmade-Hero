#pragma once

#include <cmath>
#include <cstdint>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// TODO(violeta): Creería que en PSX esto no crashea el programa.
#ifdef DEBUG
#define Assert(expression) \
    if (!(expression)) {   \
        *(int*)0 = 0;      \
    }
#else
#define Assert(expression)
#endif

#define internal static
#define local_persist static
#define global_variable static

#define PI 3.14159274f
#define TAU 6.28318548f

#define PI64 3.1415926535897931
#define TAU64 6.2831853071795862

#define KB(val) ((val) * 1024)
#define MB(val) (KB(val) * 1024)
#define GB(val) (MB(val) * 1024)

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef int32_t b32;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef double_t f64;
typedef float_t  f32;

struct v2 {
    f32 x, y;
};

union v2i {
    struct {
        i32 x, y;
    };
    struct {
        i32 width, height;
    };
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

struct GameState {
    f64 delta;
    f64 testPitch;
    v2i pos[2];
};

// TODO(violeta): Remove
struct TestSound {
    u8* buf;
    u32 byteCount;
    u32 sampleCount;
};

struct SoundBuffer {
    TestSound testSound;

    u32 sampleRate;
    u32 bitRate;
};

struct ScreenBuffer {
    u8* Memory;
    i32 Width, Height;
    i32 BytesPerPixel;
};

#define GAME_UPDATE(name) \
    void name(Memory* memory, InputBuffer* input, ScreenBuffer* screen, SoundBuffer* sound)
typedef GAME_UPDATE(GameUpdate);
extern "C" GAME_UPDATE(GameUpdateStub) {}

#define GAME_INIT(name) void name(Memory* memory)
typedef GAME_INIT(GameInit);
extern "C" GAME_INIT(GameInitStub) {}
