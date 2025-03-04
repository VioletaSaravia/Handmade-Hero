#pragma once

#include <cmath>
#include <cstdint>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define UNREACHABLE (*(int*)0 = 0)

#ifdef HANDMADE_INTERNAL
#define Assert(expression) \
    if (!(expression)) {   \
        UNREACHABLE;       \
    }
#else
#define Assert(expression)
#endif

#define internal static
#define local_persist static
#define global static

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

struct Shader {
    u32 ID;
};
Shader InitShader(const char* vertFile, const char* fragFile);
void   ShaderSetInt(Shader shader, const char* name, i32 value);
void   ShaderSetFloat(Shader shader, const char* name, f32 value);
void   ShaderSetFloat2(Shader shader, const char* name, v2 value);

global Shader DefaultShader;

struct Texture {
    u32 id;
    i32 width, height, nChannels;
};
Texture LoadTexture(const char* path);

struct Object {
    u32     VAO;
    Shader  shader;
    Texture texture;
};
Object InitObject(Texture texture, Shader shader, const f32* vertices, u32 vSize,
                  const u32* indices, u32 iSize);
void   DrawObject3D(Object obj);

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
