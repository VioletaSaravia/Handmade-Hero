#pragma once

#include <Windows.h>
#include <Xinput.h>
#include <glad.h>

#undef DrawText
#undef PlaySound
#include "engine.h"

#include "stb_image.h"
#include "miniaudio.h"

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

typedef struct GraphicsCtx {
    Shader      shaders[SHADER_COUNT];
    Framebuffer postprocessing;
    u32         activeShader;
    Mesh        squareMesh;
    Texture     mouse;
    Tilemap     textBox;
    Camera      cam;
} GraphicsCtx;
internal void *Win32GetProcAddress(const char *name);
internal void  InitOpenGL(HWND hWnd, const GameSettings *settings);
internal void TimeAndRender(TimingCtx *timing, const WindowCtx *window, const GraphicsCtx *graphics,
                            bool mouse);

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

#define MAX_SOUNDS 32
typedef struct AudioCtx {
    ma_device        device;
    ma_device_config config;
    SoundBuffer      sounds[MAX_SOUNDS];
    u32              count;
} AudioCtx;

typedef struct TimingCtx {
    f32  delta, targetSpf;
    i64  lastCounter, lastCycleCount, perfCountFreq;
    u32  desiredSchedulerMs;
    bool granularSleepOn;
} TimingCtx;
internal i64 GetWallClock();
internal f32 GetSecondsElapsed(i64 perfCountFreq, i64 start, i64 end);

typedef struct WindowCtx {
    bool       running;
    bool       fullscreen;
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

typedef struct EngineCtx {
    GameSettings Settings;
    InputCtx     Input;
    TimingCtx    Timing;
    WindowCtx    Window;
    AudioCtx     Audio;
    GraphicsCtx  Graphics;
    GameProcs    Game;
} EngineCtx;
