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

inline InputCtx *Input() {
    return &E->Input;
}
inline WindowCtx *Window() {
    return &E->Window;
}
inline GraphicsCtx *Graphics() {
    return &E->Graphics;
}
inline AudioCtx *Audio() {
    return &E->Audio;
}
inline TimingCtx *Timing() {
    return &E->Timing;
}
inline GameSettings *Settings() {
    return &E->Settings;
}

inline f32 Delta() {
    return Timing()->delta;
}
inline u64 Time() {
    return Timing()->time;
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

    E->Game.Setup();
    E->Memory   = NewArena((void *)((u8 *)E + sizeof(EngineCtx)), 128 * 1'000'000);
    E->Window   = InitWindow();
    E->Graphics = InitGraphics(&E->Window, &E->Settings);
    InitAudio(&E->Audio);
    E->Timing = InitTiming(SDL_GetCurrentDisplayMode(SDL_GetDisplays(0)[0])->refresh_rate);

    E->Game.Init();
}

export void EngineUpdate() {
    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT: Window()->quit = true; break;
        default: break;
        }
    }

    InputCtx *input   = Input();
    input->mouse.prev = input->mouse.cur;
    input->mouse.cur  = SDL_GetMouseState(&input->mouse.pos.x, &input->mouse.pos.y);

    TimingCtx *timing = &E->Timing;
    E->Timing.time    = SDL_GetTicks();
    timing->delta = GetSecondsElapsed(timing->perfFreq, timing->last, SDL_GetPerformanceCounter());

    while (timing->delta < timing->targetSpf) {
        SDL_Delay((u32)(1000.0 * (timing->targetSpf - timing->delta)));
        timing->delta = GetSecondsElapsed(SDL_GetPerformanceFrequency(), timing->last,
                                          SDL_GetPerformanceCounter());
    }

    f32 msPerFrame = timing->delta * 1000.0f;
    f32 msBehind   = (timing->delta - timing->targetSpf) * 1000.0f;
    f64 fps        = (f64)(timing->perfFreq) / (f64)(SDL_GetPerformanceCounter() - timing->last);
    LOG_INFO("FPS: %.2f MsPF: %.2f Ms behind: %.4f", fps, msPerFrame, msBehind);

    timing->last = timing->now;
    timing->now  = SDL_GetPerformanceCounter();

    E->Game.Update();

    GraphicsCtx *graphics = &E->Graphics;
    ShaderReload(&graphics->postprocessing.shader);
    for (i32 i = 0; i < SHADER_COUNT; i++) ShaderReload(&graphics->builtinShaders[i]);
    Framebufferuse(graphics->postprocessing);
    {
        ClearScreen((v4){0.3f, 0.4f, 0.4f, 1.0f});
        E->Game.Draw();
        CameraEnd();
        if (!Settings()->disableMouse) DrawMouse();
    }
    FramebufferDraw(graphics->postprocessing);

    WindowCtx *window = &E->Window;
    SDL_GL_SwapWindow(window->window);
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

WindowCtx InitWindow() {
    WindowCtx buffer = {0};

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
    SDL_SetAppMetadata("Handmade", "0.1", "com.violeta.handmade");
    SDL_SetAppMetadataProperty("SDL_PROP_APP_METADATA_CREATOR_STRING", "Violeta Saravia");
    // SDL_CreateWindowAndRenderer("Handmade v0.1", 640, 360, SDL_WINDOW_OPENGL, &buffer.window,
    //                             &buffer.renderer);
    buffer.window = SDL_CreateWindow("Handmade v0.1", 640, 360, SDL_WINDOW_OPENGL);
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
    glViewport(0, 0, 640, 360);
    if (!SDL_ShowWindow(buffer.window)) LOG_FATAL("Failed to show window: %s", SDL_GetError())

    return buffer;
}

TimingCtx InitTiming(f32 refreshRate) {
    // timeBeginPeriod(1);
    TimingCtx result = {
        .delta     = 1.0f / refreshRate,
        .targetSpf = result.delta,
        .last      = 0,
        .now       = SDL_GetPerformanceCounter(),
        .perfFreq  = SDL_GetPerformanceFrequency(),
    };
    return result;
}
