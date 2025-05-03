#pragma once

#include "common.h"

typedef struct {
    cstr name, version;
    v2i  resolution;
    bool disableMouse;
    bool fullscreen;
} GameSettings;
GameSettings *Settings();

typedef struct InputCtx InputCtx;
InputCtx               *Input();

typedef struct WindowCtx {
    SDL_Window   *window;
    SDL_GLContext glCtx;
    bool          fullscreen, quit;
} WindowCtx;
WindowCtx       *Window();
intern WindowCtx InitWindow(const GameSettings *settings);
void             UpdateWindow(WindowCtx *ctx);

typedef struct GraphicsCtx GraphicsCtx;
GraphicsCtx               *Graphics();

typedef struct {
    f32 delta, targetSpf;
    u64 time, now, last, perfFreq;
} TimingCtx;
intern TimingCtx InitTiming(f32 refreshRate);
void             UpdateTiming(TimingCtx *ctx);
f32              Delta();
u64              Time();
inline f32       GetSecondsElapsed(u64 perfCountFreq, u64 start, u64 end) {
    return (f32)(end - start) / (f32)(perfCountFreq);
}

typedef struct EngineCtx EngineCtx;
typedef struct GameState GameState;

typedef struct {
    void (*Setup)();
    void (*Init)();
    void (*Update)();
    void (*Draw)();
} GameCode;

// TODO Windows specific export keyword
export void  EngineLoadGame(void (*setup)(), void (*init)(), void (*update)(), void (*draw)());
export void  EngineReloadMemory(void *memory);
export void  EngineInit();
export void  EngineUpdate();
export void  EngineShutdown();
export void *EngineGetMemory();
export bool  EngineIsRunning();

Arena *Memory();
f32    Delta();
u64    Time();
