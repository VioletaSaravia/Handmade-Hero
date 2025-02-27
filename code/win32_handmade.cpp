#include <strsafe.h>  // StringCbPrintfA
#include <Windows.h>
#include <xaudio2.h>
#include <Xinput.h>

#include "handmade.h"

/*  TODO: PLATFORM LAYER LIST (INCOMPLETE):

    - Savegame locations
    - getting handle to executable file
    - asset loading path
    - threading
    - raw input (multiple keyb support?)
    - sleep/timebegin period
    - ClipCursor() (for multi monitor)
    - fullscreen support
    - WM_SETCURSOR (cursor visibility)
    - QueryCancelAutoplay()
    - WM_ACTIVATEAPP (for when app is not active)
    - blit speed improvements (bitblt)
    - hardware acceleration (opengl/d3d/vulkan/etc.)
    - GetKeyboardLayout()
*/

// <Debug>
#ifndef DEBUG
#define WIN32_CHECK(func) (func)
#else
#define WIN32_CHECK(func)                                                        \
    if (FAILED(func)) {                                                          \
        char buf[32];                                                            \
        StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %d\n", GetLastError()); \
        OutputDebugStringA(buf);                                                 \
    }
#endif

#ifndef DEBUG
#define WIN32_LOG(str)
#else
#define WIN32_LOG(str)                                            \
    char buf[128];                                                \
    StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %s\n", str); \
    OutputDebugStringA(buf);
#endif

void *PlatformReadEntireFile(const char *filename) {
    HANDLE handle =
        CreateFileA(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
    Assert(handle != INVALID_HANDLE_VALUE);

    LARGE_INTEGER size     = {};
    LARGE_INTEGER sizeRead = {};

    bool ok = GetFileSizeEx(handle, &size);
    Assert(ok);
    Assert(size.HighPart == 0);  // 4GB+ reads not supported >:(

    void *result = VirtualAlloc(0, size.LowPart, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    ok = ReadFile(handle, result, size.LowPart, &sizeRead.LowPart, 0);
    Assert(ok);

    CloseHandle(handle);

    Assert(sizeRead == size);

    return result;
};

// TODO(violeta): Untested
bool PlatformWriteEntireFile(char *filename, u32 size, void *memory) {
    HANDLE handle =
        CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (handle == INVALID_HANDLE_VALUE) {
        WIN32_LOG("Error open file for writing");
        return false;
    }

    DWORD sizeWritten = 0;

    bool ok = WriteFile(handle, memory, size, &sizeWritten, 0);

    CloseHandle(handle);

    if (!ok || sizeWritten != size) {
        WIN32_LOG("Error writing file");
        return false;
    }

    return true;
};

void PlatformFreeFileMemory(void *memory) { VirtualFree(memory, 0, MEM_RELEASE); };
// </Debug>

struct Win32GameCode {
    HMODULE     dll;
    GameInit   *Init;
    GameUpdate *Update;
};

internal Win32GameCode Win32LoadGame() {
    Win32GameCode result = {};

    result.dll    = LoadLibraryA("../build/handmade.dll");
    result.Init   = GameInitStub;
    result.Update = GameUpdateStub;

    if (!result.dll) {
        OutputDebugStringA("[WIN32 ERROR] handmade.dll not loaded");
    } else {
        result.Init   = (GameInit *)GetProcAddress(result.dll, "game_init");
        result.Update = (GameUpdate *)GetProcAddress(result.dll, "game_update");
    }

    return result;
}

internal void Win32UnloadGame(Win32GameCode *game) {
    if (game->dll) FreeLibrary(game->dll);

    game->Init   = GameInitStub;
    game->Update = GameUpdateStub;
}

struct Win32ScreenBuffer : ScreenBuffer {
    HWND       window;
    HDC        deviceContext;
    DWORD      refreshRate;
    i64        perfCountFrequency;
    BITMAPINFO Info;
};
global_variable Win32ScreenBuffer Win32Screen;

struct Win32InputBuffer : InputBuffer {};
global_variable Win32InputBuffer Win32Input;

global_variable Memory Win32Memory;

global_variable bool Running = true;

struct Win32SoundBuffer : SoundBuffer {
    IXAudio2               *xAudio2;
    IXAudio2MasteringVoice *xAudio2MasteringVoice;
    IXAudio2SourceVoice    *xAudio2TestSourceVoice;
};
global_variable Win32SoundBuffer Win32Sound;

internal Win32SoundBuffer Win32InitSound() {
    Win32SoundBuffer sound = {};
    WIN32_CHECK(CoInitializeEx(0, COINIT_MULTITHREADED));

    WIN32_CHECK(XAudio2Create(&sound.xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR));
    WIN32_CHECK(sound.xAudio2->CreateMasteringVoice(&sound.xAudio2MasteringVoice));

    sound.sampleRate = 44100;
    sound.bitRate    = 16;

    // <Test Sine>

    u32 cyclesPerSec       = 220.0;
    f32 samplesPerCycle    = u32(sound.sampleRate / cyclesPerSec);
    u16 bufferSizeInCycles = 10;

    sound.testSound.sampleCount = samplesPerCycle * bufferSizeInCycles;
    sound.testSound.byteCount   = sound.testSound.sampleCount * sound.bitRate / 8;
    sound.testSound.buf =
        (u8 *)VirtualAlloc(0, sound.testSound.byteCount, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    WAVEFORMATEX waveFormat    = {};
    waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
    waveFormat.nChannels       = 1;
    waveFormat.nSamplesPerSec  = sound.sampleRate;
    waveFormat.nBlockAlign     = waveFormat.nChannels * sound.bitRate / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.wBitsPerSample  = sound.bitRate;
    waveFormat.cbSize          = 0;

    WIN32_CHECK(sound.xAudio2->CreateSourceVoice(&sound.xAudio2TestSourceVoice, &waveFormat));

    double   phase{};
    uint32_t bufferIndex{};
    while (bufferIndex < sound.testSound.byteCount) {
        phase += (2 * PI) / samplesPerCycle;
        int16_t sample                     = (int16_t)(sin(phase) * INT16_MAX * 0.5);
        sound.testSound.buf[bufferIndex++] = (byte)sample;  // Values are little-endian.
        sound.testSound.buf[bufferIndex++] = (byte)(sample >> 8);
    }

    XAUDIO2_BUFFER xAudio2Buffer = {
        .Flags      = XAUDIO2_END_OF_STREAM,
        .AudioBytes = sound.testSound.byteCount,
        .pAudioData = sound.testSound.buf,
        .PlayBegin  = 0,
        .PlayLength = 0,
        .LoopBegin  = 0,
        .LoopLength = 0,
        .LoopCount  = XAUDIO2_LOOP_INFINITE,
    };

    WIN32_CHECK(sound.xAudio2TestSourceVoice->SubmitSourceBuffer(&xAudio2Buffer));
    WIN32_CHECK(sound.xAudio2TestSourceVoice->Start());
    // </Test Sine>

    return sound;
}

internal v2i GetWindowDimension(HWND window) {
    RECT clientRect;  // Rect of "client" (drawable area)
    GetClientRect(window, &clientRect);
    return {{clientRect.right - clientRect.left, clientRect.bottom - clientRect.top}};
}

// DIB = Device-independent bitmap
internal void Win32ResizeDIBSection(Win32ScreenBuffer *buffer, int width, int height) {
    if (buffer->Memory) {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width         = width;
    buffer->Height        = height;
    buffer->BytesPerPixel = 4;

    buffer->Info = {
        .bmiHeader = {.biSize  = sizeof(buffer->Info.bmiHeader),
                      .biWidth = buffer->Width,
                      // Negative height tells Windows to treat the window's y axis as top-down
                      .biHeight        = -buffer->Height,
                      .biPlanes        = 1,
                      .biBitCount      = 32,  // 4 byte align
                      .biCompression   = BI_RGB,
                      .biSizeImage     = 0,
                      .biXPelsPerMeter = 0,
                      .biYPelsPerMeter = 0,
                      .biClrUsed       = 0,
                      .biClrImportant  = 0}};

    u64 bitmapSize = u64(buffer->Width * buffer->Height) * buffer->BytesPerPixel;
    buffer->Memory = (u8 *)VirtualAlloc(0, bitmapSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal void Win32DisplayBuffer(Win32ScreenBuffer buffer, HDC deviceContext, int windowWidth,
                                 int windowHeight) {
    // TODO(violeta): Aspect ratio correction and stretch modes
    if (!StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer.Width,
                       buffer.Height, buffer.Memory, &buffer.Info, DIB_RGB_COLORS, SRCCOPY)) {
        WIN32_LOG("Error displaying buffer");
    };
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE: {
        } break;

        case WM_DESTROY: {
            WIN32_LOG("Window destroyed");
            Running = false;
        } break;

        case WM_CLOSE: {
            Running = false;
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC         deviceContext = BeginPaint(window, &paint);

            auto dim = GetWindowDimension(window);
            Win32DisplayBuffer(Win32Screen, deviceContext, dim.width, dim.height);

            EndPaint(window, &paint);
        } break;

        case WM_ACTIVATEAPP: {
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            bool wasDown = (lParam & (1 << 30));
            bool isDown  = (lParam & (1 << 31)) == 0;

            Win32Input.keyboard.keys[wParam] = wasDown && isDown    ? ButtonState::Pressed
                                               : !wasDown && isDown ? ButtonState::JustPressed
                                                                    : ButtonState::JustReleased;
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }

    return result;
}

#define MAX_CONTROLLERS (MIN(XUSER_MAX_COUNT, CONTROLLER_COUNT))

internal void Win32ProcessXInputControllers(InputBuffer *state) {
    for (DWORD i = 0; i < MAX_CONTROLLERS; i++) {
        ControllerState  newController = {};
        ControllerState *controller    = &state->controllers[i];

        XINPUT_STATE xInputState = {};
        if (XInputGetState(i, &xInputState) != ERROR_SUCCESS) {
            if (controller->connected) {
                OutputDebugStringA("[Controller TODO] Disconected");
                controller->connected = false;
            }
            continue;
        } else {
            if (!controller->connected) {
                controller->connected = true;
                OutputDebugStringA("[Controller TODO] Connected");
            }
        }

        XINPUT_GAMEPAD *pad = &xInputState.Gamepad;

        i32 btnState[GamepadButton::COUNT] = {
            (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP),
            (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN),
            (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT),
            (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT),
            (pad->wButtons & XINPUT_GAMEPAD_A),
            (pad->wButtons & XINPUT_GAMEPAD_B),
            (pad->wButtons & XINPUT_GAMEPAD_X),
            (pad->wButtons & XINPUT_GAMEPAD_Y),
            (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER),
            (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER),
            (pad->wButtons & XINPUT_GAMEPAD_START),
            (pad->wButtons & XINPUT_GAMEPAD_BACK),
            (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB),
            (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB),
        };

        for (i32 j = 0; j < GamepadButton::COUNT; j++) {
            switch (controller->buttons[j]) {
                case ButtonState::Pressed:
                case ButtonState::JustPressed:
                    newController.buttons[j] =
                        btnState[j] != 0 ? ButtonState::Pressed : ButtonState::JustReleased;
                    break;

                case ButtonState::Released:
                case ButtonState::JustReleased:
                    newController.buttons[j] =
                        btnState[j] != 0 ? ButtonState::JustPressed : ButtonState::Released;
                    break;
            }
        }

        newController.analogLStart = controller->analogLEnd;
        newController.analogRStart = controller->analogREnd;

        f32 stickLX = f32(pad->sThumbLX) / (pad->sThumbLX < 0 ? -/* -? */ 32768.0f : 32767.0f);
        f32 stickLY = f32(pad->sThumbLY) / (pad->sThumbLY < 0 ? -/* -? */ 32768.0f : 32767.0f);
        f32 stickRX = f32(pad->sThumbRX) / (pad->sThumbRX < 0 ? -/* -? */ 32768.0f : 32767.0f);
        f32 stickRY = f32(pad->sThumbRY) / (pad->sThumbRY < 0 ? -/* -? */ 32768.0f : 32767.0f);

        newController.analogLEnd = {stickLX, stickLY};
        newController.analogREnd = {stickRX, stickRY};

        newController.triggerLStart = controller->triggerLEnd;
        newController.triggerRStart = controller->triggerREnd;

        newController.triggerLEnd = f32(pad->bLeftTrigger) / 255.0f;
        newController.triggerREnd = f32(pad->bRightTrigger) / 255.0f;

        *controller = newController;
    }
}

internal DWORD Win32GetRefreshRate(HWND window) {
    HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

    MONITORINFOEXA monitorInfo = {};
    monitorInfo.cbSize         = sizeof(MONITORINFOEXA);
    if (!GetMonitorInfoA(monitor, &monitorInfo))
        OutputDebugStringA("[WIN32 ERROR] Couldn't get monitor info");

    DEVMODEA dm = {};
    dm.dmSize   = sizeof(DEVMODEA);
    if (!EnumDisplaySettingsExA(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &dm, EDS_RAWMODE))
        OutputDebugStringA("[WIN32 ERROR] Couldn't get monitor refresh rate");

    return dm.dmDisplayFrequency;
}

internal Win32ScreenBuffer Win32InitWindow(HINSTANCE instance) {
    Win32ScreenBuffer screen = {};
    Win32ResizeDIBSection(&screen, 1280, 720);

    WNDCLASSA windowClass = {
        .style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc   = Win32MainWindowCallback,
        .hInstance     = instance,
        .hIcon         = 0,
        .lpszClassName = "HandmadeHeroClassName",
    };

    if (!RegisterClassA(&windowClass)) {
        OutputDebugStringA("[ERROR] Couldn't register window class");
        return {};
    }

    screen.window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
    if (!screen.window) {
        OutputDebugStringA("[ERROR] Couldn't create window");
        return {};
    }
    screen.deviceContext = GetDC(screen.window);

    screen.refreshRate = Win32GetRefreshRate(screen.window);

    return screen;
}

internal i64 Win32GetWallClock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
}

internal i64 Win32InitPerformanceCounter(i64 *freq) {
    LARGE_INTEGER perfCountFrequencyResult = {};
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    *freq = perfCountFrequencyResult.QuadPart;

    return Win32GetWallClock();
}

internal f64 Win32GetSecondsElapsed(i64 start, i64 end) {
    return f64(end - start) / f64(Win32Screen.perfCountFrequency);
}

global_variable Win32GameCode Win32Game;

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
    // <Init>
    Win32Game   = Win32LoadGame();
    Win32Screen = Win32InitWindow(instance);
    Win32Sound  = Win32InitSound();

    Win32Memory = {
        .permStoreSize    = MB(64),
        .permStore        = VirtualAlloc(0, MB(64), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE),
        .scratchStoreSize = MB(64),
        .scratchStore     = VirtualAlloc(0, MB(64), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE),
    };
    Win32Game.Init(&Win32Memory);
    // </Init>

    // <Timing>
    // Used by Sleep()
    u32  desiredSchedulerMs = 1;
    bool granularSleepOn    = timeBeginPeriod(desiredSchedulerMs);

    f32 targetSPF      = 1.0f / Win32Screen.refreshRate;
    i64 lastCounter    = Win32InitPerformanceCounter(&Win32Screen.perfCountFrequency);
    u64 lastCycleCount = __rdtsc();
    // </Timing>

    while (Running) {
        // <Input>

        // Win32ProcessKeyboard(&Win32Input);
        for (i32 i = 0; i < KEY_COUNT; i++) {
            if (Win32Input.keyboard.keys[i] == ButtonState::JustReleased)
                Win32Input.keyboard.keys[i] = ButtonState::Released;
        }

        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                Running = false;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        Win32ProcessXInputControllers(&Win32Input);
        // </Input>

        // MAIN UPDATE
        Win32Game.Update(&Win32Memory, &Win32Input, &Win32Screen, &Win32Sound);

        // <Timing>
        u64 endCycleCount      = __rdtsc();
        u64 cyclesElapsed      = endCycleCount - lastCycleCount;
        f64 megaCyclesPerFrame = f64(cyclesElapsed) / (1000.0 * 1000.0);

        f64 deltaSecs = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
        while (deltaSecs < targetSPF) {
            if (granularSleepOn) Sleep(DWORD(1000.0 * (targetSPF - deltaSecs)));
            deltaSecs = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
        }
        f64 msPerFrame = deltaSecs * 1000.0;
        f64 msBehind   = (deltaSecs - targetSPF) * 1000.0;
        f64 fps = f64(Win32Screen.perfCountFrequency) / f64(Win32GetWallClock() - lastCounter);

        // <Render>
        v2i dim = GetWindowDimension(Win32Screen.window);
        Win32DisplayBuffer(Win32Screen, Win32Screen.deviceContext, dim.width, dim.height);
        ReleaseDC(Win32Screen.window, Win32Screen.deviceContext);
        // </Render>

        char debugBuf[128];
        WIN32_CHECK(StringCbPrintfA(debugBuf, sizeof(debugBuf),
                                    "%.2f ms/f (%.2f ms behind target), %.2f fps, %.2f mc/f\n",
                                    msPerFrame, msBehind, fps, megaCyclesPerFrame));
        OutputDebugStringA(debugBuf);

        lastCounter    = Win32GetWallClock();
        lastCycleCount = endCycleCount;
        // </Timing>
    }

    return 0;
}