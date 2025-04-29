#include "engine.h"

#include "audio.h"
#include "common.h"
#include "graphics.h"
#include "input.h"

struct EngineCtx {
    Arena        Memory;
    GameSettings Settings;
    InputCtx     Input;
    TimingCtx    Timing;
    WindowCtx    Window;
    AudioCtx     Audio;
    GraphicsCtx  Graphics;
    GameCode     Game;
};

EngineCtx *E;
GameState *S;

InputCtx *Input() {
    return &E->Input;
}
WindowCtx *Window() {
    return &E->Window;
}
GraphicsCtx *Graphics() {
    return &E->Graphics;
}
AudioCtx *Audio() {
    return &E->Audio;
}
TimingCtx *Timing() {
    return &E->Timing;
}
GameSettings *Settings() {
    return &E->Settings;
}

f32 Delta() {
    return Timing()->delta;
}
u64 Time() {
    return Timing()->time;
}
v2 Mouse() {
    return Input()->mousePos;
}

export void EngineLoadGame(void (*setup)(), void (*init)(), void (*update)(), void (*draw)()) {
    E->Game.Setup  = setup;
    E->Game.Init   = init;
    E->Game.Update = update;
    E->Game.Draw   = draw;
}

export void EngineInit() {
    SDL_srand(0);

    E->Game.Setup();
    E->Memory   = NewArena((void *)((u8 *)E + sizeof(EngineCtx)), 128 * 1'000'000);
    E->Window   = InitWindow(Settings());
    E->Graphics = InitGraphics(&E->Window, &E->Settings);
    E->Audio    = InitAudio();
    E->Timing   = InitTiming(SDL_GetCurrentDisplayMode(SDL_GetDisplays(0)[0])->refresh_rate);
    E->Input    = InitInput();

    E->Game.Init();
}

export void EngineUpdate() {
    UpdateInput(Input());
    UpdateTiming(Timing());

    E->Game.Update();

    UpdateGraphics(Graphics(), E->Game.Draw);
    UpdateWindow(Window());
}

extern void EngineReloadMemory(void *memory) {
    E = memory;
    S = (GameState *)((u8 *)memory + sizeof(EngineCtx));
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        return;
    }
}

export void EngineShutdown() {
    ShutdownAudio(Audio());
    SDL_GL_DestroyContext(Window()->glCtx);
    SDL_DestroyWindow(Window()->window);
    SDL_Quit();
}

export void *EngineGetMemory() {
    if (E)
        return E;
    else
        LOG_FATAL("Engine context not loaded");
}

export bool EngineIsRunning() {
    return !Window()->quit;
}

WindowCtx InitWindow(const GameSettings *settings) {
    WindowCtx buffer = {0};

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
    SDL_SetAppMetadata(settings->name, settings->version, "com.violeta.handmade");
    SDL_SetAppMetadataProperty("SDL_PROP_APP_METADATA_CREATOR_STRING", "Violeta Saravia");

    buffer.window = SDL_CreateWindow(settings->name, settings->resolution.w, settings->resolution.h,
                                     SDL_WINDOW_OPENGL);
    SDL_SetWindowPosition(buffer.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (E->Settings.fullscreen) SDL_SetWindowFullscreen(buffer.window, true);
    SDL_SetWindowIcon(buffer.window, IMG_Load("data\\icon.ico"));

    SDL_HideCursor();

    buffer.glCtx = SDL_GL_CreateContext(buffer.window);
    if (!SDL_GL_MakeCurrent(buffer.window, buffer.glCtx))
        LOG_FATAL("Failed to show window: %s", SDL_GetError())
    SDL_GL_SetSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) LOG_FATAL("Glad failed to load GL")

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, settings->resolution.w, settings->resolution.h);

    if (!SDL_ShowWindow(buffer.window)) LOG_FATAL("Failed to show window: %s", SDL_GetError())

    return buffer;
}

void UpdateWindow(WindowCtx *ctx) {
    SDL_GL_SwapWindow(ctx->window);
    if (GetKey(KEY_F11) == JustPressed) {
        if (SDL_SetWindowFullscreen(ctx->window, !ctx->fullscreen))
            ctx->fullscreen ^= true;
        else
            LOG_ERROR("Couldn't fullscreen window: %s", SDL_GetError());
    }
}

TimingCtx InitTiming(f32 refreshRate) {
    TimingCtx result = {
        .delta     = 1.0f / refreshRate,
        .targetSpf = result.delta,
        .last      = 0,
        .now       = SDL_GetPerformanceCounter(),
        .perfFreq  = SDL_GetPerformanceFrequency(),
    };
    return result;
}

void UpdateTiming(TimingCtx *ctx) {
    ctx->time  = SDL_GetTicks();
    ctx->delta = GetSecondsElapsed(ctx->perfFreq, ctx->last, SDL_GetPerformanceCounter());

    while (ctx->delta < ctx->targetSpf) {
        SDL_Delay((u32)(1000.0 * (ctx->targetSpf - ctx->delta)));
        ctx->delta = GetSecondsElapsed(SDL_GetPerformanceFrequency(), ctx->last,
                                       SDL_GetPerformanceCounter());
    }

    f32 msPerFrame = ctx->delta * 1000.0f;
    f32 msBehind   = (ctx->delta - ctx->targetSpf) * 1000.0f;
    f64 fps        = (f64)(ctx->perfFreq) / (f64)(SDL_GetPerformanceCounter() - ctx->last);
    // LOG_INFO("FPS: %.2f MsPF: %.2f Ms behind: %.4f", fps, msPerFrame, msBehind);

    ctx->last = ctx->now;
    ctx->now  = SDL_GetPerformanceCounter();
}
