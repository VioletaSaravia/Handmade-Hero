#include "win32.h"

#include "audio.c"
#include "graphics.c"
#include "input.c"

EngineCtx *E;
GameState *S;

#define RAW_ALLOC(size) VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
#define RAW_FREE(ptr) VirtualFree(ptr, 0, MEM_RELEASE)
#undef ALLOC
#define ALLOC(size) Alloc(&E->Memory, size)
#undef FREE
#define FREE(ptr) DeAlloc(&E->Memory, ptr)

Arena NewArena(void *memory, u64 size) {
    return (Arena){.buf = memory, .used = 0, .size = size};
}

void *Alloc(Arena *arena, u64 size) {
    if (arena->used + size >= arena->size) {
        LOG_ERROR("Arena is full");
        return 0;
    }

    void *result = (void *)(&arena->buf[arena->used]);
    arena->used += size;
    return result;
}

void *RingAlloc(Arena *arena, u64 size) {
    if (arena->used + size >= arena->size) {
        arena->used = 0;
    }

    void *result = (void *)(&arena->buf[arena->used]);
    arena->used += size;
    return result;
}

void DeAlloc(Arena *arena, void *ptr) {
    i64 ptrDiff = (u8 *)ptr - arena->buf;

    // TODO is this correct?
    if (ptrDiff < 0 || ptrDiff >= (u64)arena->buf + arena->size) {
        LOG_ERROR("Pointer is not in arena");
        return;
    }

    arena->used = ptrDiff;
}

void Empty(Arena *arena) {
    arena->used = 0;
}

ComponentTable NewComponentTable(u32 buckets, u32 entSize) {
    return (ComponentTable){
        .Hash    = SimpleHash,
        .data    = RAW_ALLOC(sizeof(Component) * 4 * buckets),
        .size    = buckets,
        .entLen  = 0,
        .entSize = entSize,
    };
}

void *CompUpsert(ComponentTable *dict, const cstr key, void *data) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0 || strcmp(key, match[i].key) == 0) {
            match[i] = (Component){.key = key, .data = data};
            return match[i].data;
        }
    }

    printf("[Warning] [%s] Key %s exceeded maximum number of collisions\n", __func__, key);
    return 0;
}
void *CompInsert(ComponentTable *dict, const cstr key, void *data) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) {
            match[i] = (Component){.key = key, .data = data};
            return match[i].data;
        }

        if (strcmp(key, match[i].key) == 0) {
            printf("[Warning] [%s] Key %s is already inserted\n", __func__, key);
            return match[i].data;
        }
    }
    printf("[Warning] [%s] Key %s exceeded maximum number of collisions\n", __func__, key);
    return 0;
}
void *CompUpdate(ComponentTable *dict, const cstr key, void *data) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) break;
        if (strcmp(key, match[i].key) == 0) {
            match[i].data = data;
            return match[i].data;
        }
    }
    printf("[Warning] [%s] Key %s not found\n", __func__, key);
    return 0;
}
void *CompGet(const ComponentTable *dict, const cstr key) {
    Component *match = dict->data[dict->Hash(key) % dict->size];
    for (size_t i = 0; i < MAX_COLLISIONS; i++) {
        if (match[i].key == 0) break;
        if (strcmp(key, match[i].key) == 0) {
            return match[i].data;
        }
    }

    printf("[Warning] [%s] Key %s not found\n", __func__, key);
    return 0;
}

// ===== IMAGE =====

Image LoadBMP32x32Image(cstr path) {
    string data   = ReadEntireFile(path);
    Image  result = {0};

    AseBMP32x32 *bmpData = (AseBMP32x32 *)data.data;
    result.data          = malloc(1024 * 4);
    for (u32 i = 0; i < 256; i++) {
        u32 iData          = i * 4;
        result.data[iData] = bmpData->rgbq[i].r;
        result.data[iData] = bmpData->rgbq[i].g;
        result.data[iData] = bmpData->rgbq[i].b;
        result.data[iData] = bmpData->rgbq[i].a;
    }
    return result;
}

// ===== ENGINE =====

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
GameSettings *Settings() {
    return &E->Settings;
}
f32 Delta() {
    return E->Timing.delta;
}
i32 Time() {
    return E->Timing.time;
}

global f32 squareVerts[] = {1, 1, 0, 1, 0, 1, -1, 0, 1, 1, -1, -1, 1, 0, 1, -1, 1, 0, 0, 0};
global u32 squareIds[]   = {0, 1, 3, 1, 2, 3};

GraphicsCtx InitGraphics(WindowCtx *ctx, const GameSettings *settings) {
    GraphicsCtx result = {0};
    InitOpenGL(ctx->window, settings);

    result.builtinShaders[SHADER_Default] = ShaderFromPath(0, 0);
    result.builtinShaders[SHADER_Tiled]   = ShaderFromPath("shaders\\tiled.vert", 0);
    result.builtinShaders[SHADER_Text] = ShaderFromPath("shaders\\text.vert", "shaders\\text.frag");
    result.builtinShaders[SHADER_Rect] = ShaderFromPath("shaders\\rect.vert", "shaders\\rect.frag");
    result.builtinShaders[SHADER_Line] = ShaderFromPath("shaders\\line.vert", "shaders\\line.frag");

    result.builtinTextures[TEX_Mouse] = NewTexture("data\\pointer.png");

    result.builtinVAOs[VAO_SQUARE]                 = VAOFromShader("shaders\\default.vert");
    result.builtinVAOs[VAO_SQUARE].objs[0].buf     = squareVerts;
    result.builtinVAOs[VAO_SQUARE].objs[0].bufSize = 20 * sizeof(f32);
    result.builtinVAOs[VAO_SQUARE].objs[1].buf     = squareIds;
    result.builtinVAOs[VAO_SQUARE].objs[1].bufSize = 6 * sizeof(i32);
    VAOInit(&result.builtinVAOs[VAO_SQUARE], 0);

    result.builtinVAOs[VAO_TEXT] = VAOFromShader("shaders\\text.vert");
    // TODO Allocs
    VAOInit(&result.builtinVAOs[VAO_TEXT], 0);

    result.postprocessing = NewFramebuffer("shaders\\post.frag");

    return result;
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
    E->Timing = InitTiming(E->Window.refreshRate);

    if (E->Settings.fullscreen) ToggleFullscreen();
    E->Game.Init();
}

void TimeAndRender(TimingCtx *timing, const WindowCtx *window, const GraphicsCtx *graphics) {
    u64 endCycleCount    = __rdtsc();
    u64 cyclesElapsed    = endCycleCount - timing->lastCycleCount;
    f64 mgCyclesPerFrame = (f64)(cyclesElapsed) / (1000.0 * 1000.0);

    timing->delta = GetSecondsElapsed(timing->perfCountFreq, timing->lastCounter, GetWallClock());
    while (timing->delta < timing->targetSpf) {
        if (timing->granularSleepOn) Sleep((u32)(1000.0 * (timing->targetSpf - timing->delta)));
        timing->delta =
            GetSecondsElapsed(timing->perfCountFreq, timing->lastCounter, GetWallClock());
    }

    timing->time = (i32)((GetWallClock() * 1000) / timing->perfCountFreq);

    // f32 msPerFrame = timing->delta * 1000.0f;
    // f32 msBehind   = (timing->delta - timing->targetSpf) * 1000.0f;
    // f64 fps        = (f64)(timing->perfCountFreq) / (f64)(GetWallClock() - timing->lastCounter);
    // printf("[Info] [%s] FPS: %.2f MsPF: %.2f Ms behind: %.4f", __func__, fps, msPerFrame,
    // msBehind);

    Framebufferuse(graphics->postprocessing);
    {
        ClearScreen((v4){0.3f, 0.4f, 0.4f, 1.0f});
        E->Game.Draw();
        CameraEnd();
        if (!Settings()->disableMouse) {
            // DrawMouse()
            ShaderUse(Graphics()->builtinShaders[SHADER_Default]);
            SetUniform2f("res", GetResolution());
            SetUniform2f("pos", Mouse());
            SetUniform1f("scale", Settings()->scale);
            Texture mouseTex = graphics->builtinTextures[TEX_Mouse];
            SetUniform2f("size", (v2){mouseTex.size.x, mouseTex.size.y});
            SetUniform4f("color", WHITE);
            TextureUse(mouseTex);
            VAOUse(graphics->builtinVAOs[VAO_SQUARE]);
            DrawInstances(1);
        }
    }
    FramebufferDraw(graphics->postprocessing);

    SwapBuffers(window->dc);
    ReleaseDC(window->window, window->dc);

    timing->lastCounter    = GetWallClock();
    timing->lastCycleCount = endCycleCount;
}

v2 Win32GetResolution(HWND window) {
    RECT clientRect = {0};
    GetClientRect(window, &clientRect);

    return (v2){clientRect.right - clientRect.left, clientRect.bottom - clientRect.top};
}

export void EngineUpdate() {
    ProcessKeyboard(E->Input.keys, &E->Window.running);
    ProcessGamepads(E->Input.gamepads);
    ProcessMouse(&E->Input.mouse);
    E->Window.resolution = Win32GetResolution(E->Window.window);

    if (E->Input.keys[KEY_F12] == JustPressed) E->Game.Init();
    if (E->Input.keys[KEY_F11] == JustPressed) ToggleFullscreen();
    if (E->Input.keys[KEY_Esc] == JustPressed) E->Window.running = 0;

    for (i32 i = 0; i < KEY_COUNT; i++) {
        ButtonState k = E->Input.keys[i];
        if (k == JustReleased) LOG_INFO("Key was released");
        if (k == JustPressed) LOG_INFO("Key was pressed");
    }

    ShaderReload(&E->Graphics.postprocessing.shader);
    for (i32 i = 0; i < SHADER_COUNT; i++) ShaderReload(&E->Graphics.builtinShaders[i]);

    E->Game.Update();
    TimeAndRender(&E->Timing, &E->Window, &E->Graphics);
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
    return E->Window.running;
}

i64 GetWallClock() {
    LARGE_INTEGER result = {0};
    QueryPerformanceCounter(&result);
    return (i64)result.QuadPart;
}

i64 InitPerformanceCounter(i64 *freq) {
    LARGE_INTEGER result = {0};
    QueryPerformanceFrequency(&result);
    *freq = (i64)(result.QuadPart);
    return GetWallClock();
}

LRESULT WINAPI MainWindowCallback(HWND window, u32 msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR:
        HCURSOR cursor = LOWORD(lParam) == HTCLIENT ? 0 : LoadCursorW(0, (LPCWSTR)IDC_ARROW);
        SetCursor(cursor);
        break;

    case WM_SIZE: break;

    case WM_DESTROY: Window()->running = false; break;

    case WM_MOUSEHWHEEL:
        // TODO mousewheel
        break;

    case WM_PAINT:
        PAINTSTRUCT paint;
        HDC         deviceCtx = BeginPaint(window, &paint);
        EndPaint(window, &paint);
        break;

    case WM_ACTIVATEAPP: break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        bool wasDown = lParam & (1 << 30);
        bool isDown  = !(lParam & (1 << 31));

        Input()->keys[wParam] = wasDown && isDown    ? Pressed
                                : !wasDown && isDown ? JustPressed
                                : wasDown && !isDown ? JustReleased
                                                     : Released;

    default: return DefWindowProcW(window, msg, wParam, lParam);
    }

    return 0;
}

void ResizeDIBSection(WindowCtx *window, v2i size) {
    if (window->memory != 0) RAW_FREE(window->memory);

    window->w          = size.w;
    window->h          = size.h;
    window->bytesPerPx = 4;
    window->info       = (BITMAPINFO){
              .bmiHeader =
            {
                      .biSize  = sizeof(window->info.bmiHeader),
                      .biWidth = window->w,
                // Negative height tells Windows to treat the window's y axis as top-down
                      .biHeight        = -window->h,
                      .biPlanes        = 1,
                      .biBitCount      = 32, // 4 byte align
                      .biCompression   = BI_RGB,
                      .biSizeImage     = 0,
                      .biXPelsPerMeter = 0,
                      .biYPelsPerMeter = 0,
                      .biClrUsed       = 0,
                      .biClrImportant  = 0,
            },
    };

    window->memoryLen = window->w * window->h * window->bytesPerPx;
    window->memory    = RAW_ALLOC(window->memoryLen);
}

u32 GetRefreshRate(HWND hWnd) {
    HMONITOR       monitor     = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEXW monitorInfo = (MONITORINFOEXW){
        .cbSize = sizeof(MONITORINFOEXW),
    };

    GetMonitorInfoW(monitor, (LPMONITORINFO)&monitorInfo);
    DEVMODEW dm = (DEVMODEW){.dmSize = sizeof(DEVMODEW)};

    EnumDisplaySettingsW(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    return dm.dmDisplayFrequency;
}

WindowCtx InitWindow() {
    WindowCtx buffer;

    RECT rect = {0, 0, Settings()->resolution.w * Settings()->scale,
                 Settings()->resolution.h * Settings()->scale};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

    v2i size = (v2i){rect.right - rect.left, rect.bottom - rect.top};
    ResizeDIBSection(&buffer, size);
    HANDLE instance = GetModuleHandleW(0);

    WNDCLASSW window_class = {
        .style       = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = MainWindowCallback,
        .hInstance   = instance,
        .hIcon       = LoadImageW(instance, L"data\\icon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE),
        .lpszClassName = (LPCWSTR)Settings()->name,
    };

    if (RegisterClassW(&window_class) == 0) LOG_FATAL("Failed to register window")

    buffer.window =
        CreateWindowExW(0, window_class.lpszClassName, (LPCWSTR)Settings()->name,
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, size.w, size.h, 0, 0, instance, 0);

    if (buffer.window == 0) LOG_FATAL("Failed to create window")

    buffer.dc           = GetDC(buffer.window);
    buffer.refreshRate  = GetRefreshRate(buffer.window);
    buffer.running      = 1;
    buffer.windowedRect = rect;
    buffer.fullscreen   = false;
    buffer.resolution   = Win32GetResolution(buffer.window);

    return buffer;
}

internal void *Win32GetProcAddress(const char *name) {
    void *p = (void *)wglGetProcAddress(name);
    if (!p || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 || p == (void *)-1) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        p              = (void *)GetProcAddress(module, name);
    }
    return p;
}

void InitOpenGL(HWND hWnd, const GameSettings *settings) {
    PIXELFORMATDESCRIPTOR desired = (PIXELFORMATDESCRIPTOR){
        .nSize      = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion   = 1,
        .dwFlags    = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cAlphaBits = 8,
    };

    HDC                   windowDC       = GetDC(hWnd);
    i32                   suggestedIndex = ChoosePixelFormat(windowDC, &desired);
    PIXELFORMATDESCRIPTOR suggested;
    SetPixelFormat(windowDC, suggestedIndex, &suggested);

    HGLRC openglrc = wglCreateContext(windowDC);
    if (wglMakeCurrent(windowDC, openglrc) == 0) return;

    if (!gladLoadGLLoader((GLADloadproc)Win32GetProcAddress)) {
        LOG_FATAL("Glad failed to load GL");
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, settings->resolution.w * settings->scale,
               settings->resolution.h * settings->scale);

    ReleaseDC(hWnd, windowDC);
}

TimingCtx InitTiming(f32 refreshRate) {
    i64       freq   = 0;
    TimingCtx result = {
        .delta              = 1.0f / refreshRate,
        .targetSpf          = result.delta,
        .lastCounter        = InitPerformanceCounter(&freq),
        .lastCycleCount     = __rdtsc(),
        .perfCountFreq      = freq,
        .desiredSchedulerMs = 1,
        .granularSleepOn    = timeBeginPeriod(1),
    };
    return result;
}

extern void EngineReloadMemory(void *memory) {
    E = memory;
    S = (GameState *)((u8 *)memory + sizeof(EngineCtx));
    if (!gladLoadGLLoader((GLADloadproc)Win32GetProcAddress)) {
        LOG_ERROR("Glad reloading failed");
        return;
    }
}

f32 GetSecondsElapsed(i64 perfCountFreq, i64 start, i64 end) {
    return (f32)(end - start) / (f32)(perfCountFreq);
}

void ResizeWindow(HWND hWnd, v2i size) {
    size.h    = size.h == 0 ? 1 : size.h;
    RECT rect = (RECT){0, 0, size.w, size.h};

    if (!Window()->fullscreen) {
        AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, false);
    }

    SetWindowPos(hWnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER);

    glViewport(0, 0, size.w, size.h);
}

// TODO: Refactorear cuando el menu esté listo.
void ToggleFullscreen() {
    HWND        window  = Window()->window;
    HMONITOR    monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi      = {.cbSize = sizeof(mi)};
    GetMonitorInfo(monitor, &mi);

    RECT *windowedRect = &Window()->windowedRect;

    if (!Window()->fullscreen) {
        GetWindowRect(window, windowedRect);

        SetWindowLongW(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        int w = mi.rcMonitor.right - mi.rcMonitor.left;
        int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

        SetWindowPos(window, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, w, h,
                     SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        glViewport(0, 0, w, h);
    } else {
        SetWindowLongW(window, GWL_STYLE,
                       WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);

        int w = windowedRect->right - windowedRect->left;
        int h = windowedRect->bottom - windowedRect->top;

        SetWindowPos(window, HWND_NOTOPMOST, windowedRect->left + 64, windowedRect->top + 64, w, h,
                     SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        RECT clientRect;
        GetClientRect(window, &clientRect);
        int cw = clientRect.right - clientRect.left;
        int ch = clientRect.bottom - clientRect.top;

        glViewport(0, 0, cw, ch);
    }

    FramebufferResize(Graphics()->postprocessing);
    Window()->fullscreen ^= true;
}

void LockCursorToWindow(HWND hWnd) {
    RECT rect = {0};
    GetClientRect(hWnd, &rect);
    POINT screenCoord = (POINT){0};
    ClientToScreen(hWnd, &screenCoord);
    rect.left = screenCoord.x;
    rect.top  = screenCoord.y;
    ClipCursor(&rect);
}

const f32 CharmapCoords[256] = {
    [' '] = 0,   ['A'] = 1,  ['B'] = 2,  ['C'] = 3,  ['D'] = 4,  ['E'] = 5,  ['F'] = 6,
    ['G'] = 7,   ['H'] = 8,  ['I'] = 9,  ['J'] = 10, ['K'] = 11, ['L'] = 12, ['M'] = 13,
    ['N'] = 14,  ['O'] = 15, ['P'] = 16, ['Q'] = 17, ['R'] = 18, ['S'] = 19, ['T'] = 20,
    ['U'] = 21,  ['V'] = 22, ['W'] = 23, ['X'] = 24, ['Y'] = 25, ['Z'] = 26, ['?'] = 27,
    ['!'] = 28,  ['.'] = 29, [','] = 30, ['-'] = 31, ['+'] = 32, ['a'] = 33, ['b'] = 34,
    ['c'] = 35,  ['d'] = 36, ['e'] = 37, ['f'] = 38, ['g'] = 39, ['h'] = 40, ['i'] = 41,
    ['j'] = 42,  ['k'] = 43, ['l'] = 44, ['m'] = 45, ['n'] = 46, ['o'] = 47, ['p'] = 48,
    ['q'] = 49,  ['r'] = 50, ['s'] = 51, ['t'] = 52, ['u'] = 53, ['v'] = 54, ['w'] = 55,
    ['x'] = 56,  ['y'] = 57, ['z'] = 58, [':'] = 59, [';'] = 60, ['"'] = 61, ['('] = 62,
    [')'] = 63,  ['@'] = 64, ['&'] = 65, ['1'] = 66, ['2'] = 67, ['3'] = 68, ['4'] = 69,
    ['5'] = 70,  ['6'] = 71, ['7'] = 72, ['8'] = 73, ['9'] = 74, ['0'] = 75, ['%'] = 76,
    ['^'] = 77,  ['*'] = 78, ['{'] = 79, ['}'] = 80, ['='] = 81, ['#'] = 82, ['/'] = 83,
    ['\\'] = 84, ['$'] = 85, [163] = 86, ['['] = 87, [']'] = 88, ['<'] = 89, ['>'] = 90,
    ['\''] = 91, ['`'] = 92, ['~'] = 93};

const i32 MonogramCoords[256] = {
    [' '] = 0,  ['!'] = 1,  ['"'] = 2,  ['#'] = 3,  ['$'] = 4,   ['%'] = 5,  ['&'] = 6,  ['\''] = 7,
    ['('] = 8,  [')'] = 9,  ['*'] = 10, ['+'] = 11, [','] = 12,  ['-'] = 13, ['.'] = 14, ['/'] = 15,
    ['0'] = 16, ['1'] = 17, ['2'] = 18, ['3'] = 19, ['4'] = 20,  ['5'] = 21, ['6'] = 22, ['7'] = 23,
    ['8'] = 24, ['9'] = 25, [':'] = 26, [';'] = 27, ['<'] = 28,  ['='] = 29, ['>'] = 30, ['?'] = 31,
    ['@'] = 32, ['A'] = 33, ['B'] = 34, ['C'] = 35, ['D'] = 36,  ['E'] = 37, ['F'] = 38, ['G'] = 39,
    ['H'] = 40, ['I'] = 41, ['J'] = 42, ['K'] = 43, ['L'] = 44,  ['M'] = 45, ['N'] = 46, ['O'] = 47,
    ['P'] = 48, ['Q'] = 49, ['R'] = 50, ['S'] = 51, ['T'] = 52,  ['U'] = 53, ['V'] = 54, ['W'] = 55,
    ['X'] = 56, ['Y'] = 57, ['Z'] = 58, ['['] = 59, ['\\'] = 60, [']'] = 61, ['^'] = 62, ['_'] = 63,
    ['`'] = 64, ['a'] = 65, ['b'] = 66, ['c'] = 67, ['d'] = 68,  ['e'] = 69, ['f'] = 70, ['g'] = 71,
    ['h'] = 72, ['i'] = 73, ['j'] = 74, ['k'] = 75, ['l'] = 76,  ['m'] = 77, ['n'] = 78, ['o'] = 79,
    ['p'] = 80, ['q'] = 81, ['r'] = 82, ['s'] = 83, ['t'] = 84,  ['u'] = 85, ['v'] = 86, ['w'] = 87,
    ['x'] = 88, ['y'] = 89, ['z'] = 90, ['{'] = 91, ['|'] = 92,  ['}'] = 93, ['~'] = 94};

u32 MapStringToTextBox(cstr text, i32 width, v2 tSize, void *buf) {
    if (!width) width = INT32_MAX;

    u64 textLen = strlen(text);
    u32 iText = 0, iBox = 0, nextSpace = 0;

    bool bold = false, italics = false;
    bool addWhitespace = false;
    f32  size          = tSize.x * tSize.y;
    while (iText < textLen && iBox < size) {
        if (!addWhitespace)
            for (u32 i = iText; i < textLen; i++) {
                if (text[i] == ' ' || (i == (textLen - 1))) {
                    nextSpace = i - iText;
                    break;
                }
            }

        addWhitespace = nextSpace >= width - (iBox % width);

        if (text[iText] == '\b') {
            bold ^= true;
            iText++;
            continue;
        }
        if (text[iText] == '\t') {
            italics ^= true;
            iText++;
            continue;
        }

        ((i32 *)buf)[iBox] = !addWhitespace ? MonogramCoords[text[iText]] : 0;
        // if (bold) tilemap->fontVbo[iBox] = 1;
        // if (italics) tilemap->fontVbo[iBox] = 2;
        // if (bold && italics) tilemap->fontVbo[iBox] = 3;

        if (!addWhitespace) iText++;
        iBox++;

        addWhitespace = addWhitespace && ((iBox % width) != 0);
    }

    if (textLen < size)
        for (i32 i = iBox; i < size; i++) {
            ((i32 *)buf)[i] = 0;
        }

    return iBox;
}

void DrawText(VAO *vao, Texture tex, v2 tSize, v2 tileSize, const cstr text, v2 pos, i32 width,
              f32 scale) {
    u32 iBox = MapStringToTextBox(text, width, tSize, vao->objs[1].buf);

    ShaderUse(Graphics()->builtinShaders[SHADER_Tiled]);

    SetUniform2f("tile_size", tSize);
    v2i tilesetSize = (v2i){
        tex.size.x / tileSize.x,
        tex.size.y / tileSize.y,
    };

    SetUniform2i("tileset_size", tilesetSize);
    SetUniform1i("width", width ? width : INT32_MAX);
    SetUniform2f("res", GetResolution());
    SetUniform2f("pos", pos);
    SetUniform1f("scale", scale ? scale * 2 : 2);
    SetUniform1i("tex0", 0);

    TextureUse(tex);

    VAOUse(*vao);

    // f32 tileSize = tilemap->size.x * tilemap->size.y;
    BOUpdate(vao->objs[1]);

    DrawInstances(iBox);
}

inline bool V2InRect(v2 pos, Rect rectangle) {
    f32 left   = fminf(rectangle.x, rectangle.x + rectangle.w);
    f32 right  = fmaxf(rectangle.x, rectangle.x + rectangle.w);
    f32 top    = fminf(rectangle.y, rectangle.y + rectangle.h);
    f32 bottom = fmaxf(rectangle.y, rectangle.y + rectangle.h);

    return (pos.x >= left && pos.x <= right && pos.y >= top && pos.y <= bottom);
}

inline bool CollisionRectRect(Rect a, Rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

void DrawRectangle(Rect rect, v4 color, f32 radius) {
    u32 err = 0;
    ShaderUse(E->Graphics.builtinShaders[SHADER_Rect]);
    SetUniform2f("pos", rect.pos);
    SetUniform2f("size", rect.size);
    SetUniform2f("res", GetResolution());
    SetUniform4f("color", color);
    SetUniform1f("radius", radius);
    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    err = glGetError();
    if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
    err = glGetError();
    if (err != GL_NO_ERROR) printf("ERROR: 0x%X\n", err);
    glBindVertexArray(0);
}

void DrawLine(v2 from, v2 to, v4 color, f32 thickness) {
    ShaderUse(E->Graphics.builtinShaders[SHADER_Line]);
    SetUniform2f("pos", from);
    SetUniform1f("thickness", thickness ? thickness : 1);
    SetUniform2f("size", v2Sub(to, from));
    SetUniform2f("res", GetResolution());
    SetUniform4f("color", color);
    glBindVertexArray(Graphics()->builtinVAOs[VAO_SQUARE].id);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
    glBindVertexArray(0);
}

void DrawPoly(Poly poly, v4 color, f32 thickness) {
    // TODO Inefficient
    for (size_t i = 0; i < poly.count - 1; i++) {
        DrawLine(poly.verts[i], poly.verts[i + 1], color, thickness);
    }
}

void DrawCircle(v2 pos, v4 color, f32 radius) {}

internal v2 Win32GetMouse() {
    POINT pt = {0};
    if (GetCursorPos(&pt) == false) return (v2){0};

    RECT rect = {0};
    // KB global
    if (GetWindowRect(Window()->window, &rect) == false) return (v2){0};

    return (v2){pt.x - rect.left - 10, pt.y - rect.top - 34};
}

// ===== FILES =====

export u64 GetLastWriteTime(cstr file) {
    u64 result = 0;

    WIN32_FILE_ATTRIBUTE_DATA fileInfo;

    if (!GetFileAttributesEx(file, GetFileExInfoStandard, &fileInfo)) {
        printf("Failed to get file attributes. Error code: %lu\n", GetLastError());
        return 0;
    }

    FILETIME writeTime = fileInfo.ftLastWriteTime;

    result = ((u64)(writeTime.dwHighDateTime) << 32) | (u64)(writeTime.dwLowDateTime);
    return result;
}

string ReadEntireFile(const char *filename) {
    string result = {0};
    HANDLE file   = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Invalid handle");
        return (string){0};
    }

    u32 size = GetFileSize(file, 0);
    if (size == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        CloseHandle(file);
        LOG_ERROR("Invalid file size");
        return (string){0};
    }

    u64 allocSize = size + 1;

    result.data = RAW_ALLOC(allocSize);
    if (!result.data) {
        LOG_ERROR("Couldn't allocate data");
        CloseHandle(file);
        return (string){0};
    }

    bool success = ReadFile(file, result.data, size, &result.len, 0);
    CloseHandle(file);

    if (!success || result.len != size) {
        LOG_ERROR("Couldn't read entire file");
        RAW_FREE(result.data);
        return (string){0};
    }
    result.data[size] = '\0';
    return result;
}