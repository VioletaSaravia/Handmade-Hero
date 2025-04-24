#pragma once

#include <Windows.h>
#include <Xinput.h>
#include <glad.h>

#undef DrawText
#undef PlaySound
#include "../engine/engine.h"

#include "miniaudio.h"
#include "stb_image.h"

typedef struct GameSettings {
    cstr name, version;
    v2i  resolution;
    f32  scale;
    bool disableMouse;
    bool fullscreen;
} GameSettings;

typedef struct InputCtx {
    GamepadState gamepads[4];
    ButtonState  keys[KEY_COUNT];
    MouseState   mouse;
} InputCtx;
#define GAMEPAD_MAX XUSER_MAX_COUNT
internal v2 Win32GetMouse();

typedef enum {
    SHADER_Default,
    SHADER_Tiled,
    SHADER_Text,
    SHADER_Rect,
    SHADER_Line,
    SHADER_COUNT,
} BuiltinShaders;

typedef enum { VAO_SQUARE, VAO_TEXT, VAO_COUNT } BuiltinVAOs;

typedef enum {
    TEX_Mouse,
    TEX_COUNT,
} BuiltinTextures;

typedef struct GraphicsCtx {
    Camera      cam;
    u32         activeShader;
    VAO         builtinVAOs[VAO_COUNT];
    Texture     builtinTextures[TEX_COUNT];
    Shader      builtinShaders[SHADER_COUNT];
    Framebuffer postprocessing;
} GraphicsCtx;
internal void *Win32GetProcAddress(const char *name);
internal void  InitOpenGL(HWND hWnd, const GameSettings *settings);
internal void  TimeAndRender(TimingCtx *timing, const WindowCtx *window,
                             const GraphicsCtx *graphics);

typedef struct Sound {
    u32 id;
} Sound;

typedef struct SoundBuffer {
    u16         *data;
    u64          dataLen;
    f32          vol, pan;
    PlaybackType type;
    bool         playing;
    ma_decoder   decoder;
} SoundBuffer;

#define MAX_SOUNDS 64
typedef struct AudioCtx {
    ma_device        device;
    ma_device_config config;
    SoundBuffer      sounds[MAX_SOUNDS];
    u32              count;
} AudioCtx;

typedef struct TimingCtx {
    f32  delta, targetSpf;
    i32  time;
    i64  lastCounter, lastCycleCount, perfCountFreq;
    u32  desiredSchedulerMs;
    bool granularSleepOn;
} TimingCtx;
internal i64 GetWallClock();
internal f32 GetSecondsElapsed(i64 perfCountFreq, i64 start, i64 end);

typedef struct WindowCtx {
    bool       running;
    bool       fullscreen;
    v2         resolution;
    u8        *memory;
    u64        memoryLen;
    i32        w, h, bytesPerPx;
    HWND       window;
    HDC        dc;
    RECT       windowedRect;
    u32        refreshRate;
    BITMAPINFO info;
    v2         mousePos;
} WindowCtx;
LRESULT WINAPI MainWindowCallback(HWND window, u32 msg, WPARAM wParam, LPARAM lParam);
internal void  ResizeDIBSection(WindowCtx *window, v2i size);
internal u32   GetRefreshRate(HWND hWnd);
internal void  ResizeWindow(HWND hWnd, v2i size);
internal void  LockCursorToWindow(HWND hWnd);
internal v2    Win32GetResolution();

typedef struct EngineCtx {
    u64          usedMemory;
    GameSettings Settings;
    InputCtx     Input;
    TimingCtx    Timing;
    WindowCtx    Window;
    AudioCtx     Audio;
    GraphicsCtx  Graphics;
    GameCode     Game;
} EngineCtx;
