#include "engine.h"

#include "audio.c"
#include "common.c"
#include "graphics.c"
#include "input.c"

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

inline f32 Delta() {
    return E->Timing.delta;
}
inline u64 Time() {
    return E->Timing.time;
}
inline v2 Mouse() {
    return Input()->mouse.pos;
}

export void EngineLoadGame(void (*setup)(), void (*init)(), void (*update)(), void (*draw)()) {
    E->Game.Setup  = setup;
    E->Game.Init   = init;
    E->Game.Update = update;
    E->Game.Draw   = draw;
}

export void EngineInit() {
    srand((u32)time(0));

    E->Memory =
        NewArena((void *)((u8 *)E + sizeof(EngineCtx)), 128 * 1'000'000 - sizeof(EngineCtx));
    E->Game.Setup();
    E->Window   = InitWindow();
    E->Graphics = InitGraphics(&E->Window, &E->Settings);
    InitAudio(&E->Audio);
    E->Timing = InitTiming(75);

    E->Game.Init();
}

export void EngineUpdate() {
    E->Timing.time = SDL_GetTicks();

    v2i res;
    SDL_GetWindowSize(E->Window.window, &res.w, &res.h);
    ShaderReload(&E->Graphics.postprocessing.shader);
    for (i32 i = 0; i < SHADER_COUNT; i++) ShaderReload(&E->Graphics.builtinShaders[i]);

    E->Game.Update();

    TimingCtx   *timing   = &E->Timing;
    WindowCtx   *window   = &E->Window;
    GraphicsCtx *graphics = &E->Graphics;

    timing->delta = GetSecondsElapsed(timing->perfFreq, timing->last, SDL_GetPerformanceCounter());
    // while (timing->delta < timing->targetSpf) {
    //     SDL_Delay((u32)(1000.0 * (timing->targetSpf - timing->delta)));
    //     timing->delta = GetSecondsElapsed(SDL_GetPerformanceFrequency(), timing->last,
    //                                       SDL_GetPerformanceCounter());
    // }

    f32 msPerFrame = timing->delta * 1000.0f;
    f32 msBehind   = (timing->delta - timing->targetSpf) * 1000.0f;
    f64 fps        = (f64)(timing->perfFreq) / (f64)(SDL_GetPerformanceCounter() - timing->last);
    // LOG_INFO("FPS: %.2f MsPF: %.2f Ms behind: %.4f", fps, msPerFrame, msBehind);

    timing->last = timing->now;
    timing->now  = SDL_GetPerformanceCounter();

    Framebufferuse(graphics->postprocessing);
    {
        ClearScreen((v4){0.3f, 0.4f, 0.4f, 1.0f});
        E->Game.Draw();
        CameraEnd();
        if (!Settings()->disableMouse) DrawMouse();
    }
    FramebufferDraw(graphics->postprocessing);

    SDL_GL_SwapWindow(Window()->window);
}

export void EngineShutdown() {
    ShutdownAudio(&E->Audio);
}

export void *EngineGetMemory() {
    if (E)
        return E;
    else
        LOG_FATAL("Engine context not loaded");
}

export bool EngineIsRunning() {
    return true;
}

WindowCtx InitWindow() {
    WindowCtx buffer = {0};

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
    SDL_SetAppMetadata("Handmade", "0.1", "com.violeta.handmade");
    SDL_SetAppMetadataProperty("SDL_PROP_APP_METADATA_CREATOR_STRING", "Violeta Saravia");
    SDL_CreateWindowAndRenderer("Handmade v0.1", 640, 360, SDL_WINDOW_OPENGL, &buffer.window,
                                &buffer.renderer);
    SDL_SetWindowPosition(buffer.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    if (E->Settings.fullscreen) SDL_SetWindowFullscreen(buffer.window, true);
    SDL_SetWindowIcon(buffer.window, IMG_Load("data\\icon.ico"));
    SDL_HideCursor();

    SDL_GLContext glContext = SDL_GL_CreateContext(buffer.window);
    SDL_GL_MakeCurrent(buffer.window, glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) LOG_FATAL("Glad failed to load GL")

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, 640, 360);
    SDL_ShowWindow(buffer.window);

    return buffer;
}

TimingCtx InitTiming(f32 refreshRate) {
    timeBeginPeriod(1);
    i64       freq   = 0;
    TimingCtx result = {
        .delta     = 1.0f / refreshRate,
        .targetSpf = result.delta,
        .last      = 0,
        .now       = SDL_GetPerformanceCounter(),
        .perfFreq  = SDL_GetPerformanceFrequency(),
    };
    return result;
}

extern void EngineReloadMemory(void *memory) {
    E = memory;
    S = (GameState *)((u8 *)memory + sizeof(EngineCtx));
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        return;
    }
}
