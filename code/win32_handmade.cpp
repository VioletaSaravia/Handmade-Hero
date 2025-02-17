#include <math.h>
#include <stdint.h>
#include <strsafe.h>
#include <Windows.h>
#include <xaudio2.h>
#include <Xinput.h>

#include "handmade.cpp"
#include "shared.h"

/*
    TODO: PLATFORM LAYER LIST (INCOMPLETE):

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

struct Win32ScreenBuffer : ScreenBuffer {
    HWND       window;
    HDC        deviceContext;
    BITMAPINFO Info;
};
global_variable Win32ScreenBuffer Win32Screen;

struct Win32SoundBuffer : SoundBuffer {
    IXAudio2               *xAudio2;
    IXAudio2MasteringVoice *xAudio2MasteringVoice;
    IXAudio2SourceVoice    *xAudio2SourceVoice;
};
global_variable Win32SoundBuffer Win32Sound;

struct Win32InputBuffer : InputBuffer {};
global_variable Win32InputBuffer Win32Input;

global_variable bool Running = true;

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

constexpr u16 BITSPERSSAMPLE           = 16;
constexpr u32 SAMPLESPERSEC            = 44100;
constexpr f64 CYCLESPERSEC             = 220.0;
constexpr u16 AUDIOBUFFERSIZEINCYCLES  = 10;
constexpr u32 SAMPLESPERCYCLE          = u32(SAMPLESPERSEC / CYCLESPERSEC);
constexpr u32 AUDIOBUFFERSIZEINSAMPLES = SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES;
constexpr u32 AUDIOBUFFERSIZEINBYTES   = AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE / 8;

internal void Win32InitSound(Win32SoundBuffer *sound) {
    WIN32_CHECK(CoInitializeEx(0, COINIT_MULTITHREADED));

    WIN32_CHECK(XAudio2Create(&sound->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR));
    WIN32_CHECK(sound->xAudio2->CreateMasteringVoice(&sound->xAudio2MasteringVoice));

    sound->samplesPerSec = SAMPLESPERSEC;
    sound->sampleCount   = AUDIOBUFFERSIZEINSAMPLES;
    sound->byteCount     = AUDIOBUFFERSIZEINBYTES;
    sound->sampleOut     = (u8 *)malloc(AUDIOBUFFERSIZEINBYTES);
    for (u32 i = 0; i < AUDIOBUFFERSIZEINBYTES; i++) {
        sound->sampleOut[i] = 0;
    }

    WAVEFORMATEX waveFormat = {
        .wFormatTag      = WAVE_FORMAT_PCM,
        .nChannels       = 2,
        .nSamplesPerSec  = sound->samplesPerSec,
        .nAvgBytesPerSec = sound->samplesPerSec * ((2 * BITSPERSSAMPLE) / 8),
        .nBlockAlign     = (2 * BITSPERSSAMPLE) / 8,
        .wBitsPerSample  = BITSPERSSAMPLE,
        .cbSize          = 0,
    };

    sound->xAudio2->CreateSourceVoice(&sound->xAudio2SourceVoice, &waveFormat);

    XAUDIO2_BUFFER xAudio2Buffer = {
        .Flags      = XAUDIO2_END_OF_STREAM,
        .AudioBytes = AUDIOBUFFERSIZEINBYTES,
        .pAudioData = sound->sampleOut,
        .PlayBegin  = 0,
        .PlayLength = 0,
        .LoopBegin  = 0,
        .LoopLength = 0,
        .LoopCount  = XAUDIO2_LOOP_INFINITE,
    };

    WIN32_CHECK(sound->xAudio2SourceVoice->SubmitSourceBuffer(&xAudio2Buffer));
    WIN32_CHECK(sound->xAudio2SourceVoice->Start(0));
}

v2i GetWindowDimension(HWND window) {
    RECT clientRect;  // Rect of "client" (drawable area)
    GetClientRect(window, &clientRect);
    return {clientRect.right - clientRect.left, clientRect.bottom - clientRect.top};
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

    u64 bitmapMemorySize = u64(buffer->Width * buffer->Height) * buffer->BytesPerPixel;
    buffer->Memory =
        (u8 *)VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal void Win32DisplayBuffer(Win32ScreenBuffer buffer, HDC deviceContext, int windowWidth,
                                 int windowHeight) {
    // TODO(violeta): Aspect ratio correction and stretch modes
    if (!StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer.Width,
                       buffer.Height, buffer.Memory, &buffer.Info, DIB_RGB_COLORS, SRCCOPY)) {
        // TODO: log
    };
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE: {
        } break;

        case WM_DESTROY: {
            // TODO(violeta): Handle as error - recreate window
            Running = false;
        } break;

        case WM_CLOSE: {
            Running = false;
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC         deviceContext = BeginPaint(window, &paint);

            auto dim = GetWindowDimension(window);
            Win32DisplayBuffer(Win32Screen, deviceContext, dim.x, dim.y);

            EndPaint(window, &paint);
        } break;

        case WM_ACTIVATEAPP: {
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            u64 vKCode  = wParam;
            b32 wasDown = (lParam & (1 << 30));
            b32 isDown  = (lParam & (1 << 31)) == 0;
            b32 altDown = lParam & (1 << 29);

            switch (vKCode) {
                case VK_UP:
                case 'W':
                    break;

                case VK_LEFT:
                case 'A':
                    break;

                case VK_DOWN:
                case 'S':
                    break;

                case VK_RIGHT:
                case 'D':
                    break;

                case VK_F4: {
                    Running = !altDown;
                } break;

                case VK_SPACE:
                    break;

                default:
                    break;
            }
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }

    return result;
}

internal void Win32ProcessXInputControllers(InputBuffer *state) {
    for (DWORD i = 0; i < XUSER_MAX_COUNT /* != MAX_CONTROLLERS?! */; i++) {
        ControllerState  newController = {};
        ControllerState *controller    = &state->controllers[i];

        XINPUT_STATE xInputState = {};
        if (XInputGetState(i, &xInputState) != ERROR_SUCCESS) {
            // TODO(violeta): Controller not connected
            continue;
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
        };

        for (i32 i = 0; i < GamepadButton::COUNT; i++) {
            switch (controller->buttons[i]) {
                case ButtonState::Pressed:
                case ButtonState::JustPressed:
                    newController.buttons[i] =
                        btnState[i] != 0 ? ButtonState::Pressed : ButtonState::JustReleased;
                    break;

                case ButtonState::Released:
                case ButtonState::JustReleased:
                    newController.buttons[i] =
                        btnState[i] != 0 ? ButtonState::JustPressed : ButtonState::Released;
                    break;
            }
        }

        newController.analogLStart = controller->analogLEnd;
        newController.analogRStart = controller->analogREnd;

        f32 stickLX = f32(pad->sThumbLX) / (pad->sThumbLX < 0 ? 32768.0f : 32767.0f);
        f32 stickLY = f32(pad->sThumbLY) / (pad->sThumbLY < 0 ? 32768.0f : 32767.0f);
        f32 stickRX = f32(pad->sThumbRX) / (pad->sThumbRX < 0 ? 32768.0f : 32767.0f);
        f32 stickRY = f32(pad->sThumbRY) / (pad->sThumbRY < 0 ? 32768.0f : 32767.0f);

        newController.analogLEnd = {stickLX, stickLY};
        newController.analogREnd = {stickRX, stickRY};

        newController.triggerLStart = controller->triggerLEnd;
        newController.triggerRStart = controller->triggerREnd;

        newController.triggerLEnd = f32(pad->bLeftTrigger) / 255.0f;
        newController.triggerREnd = f32(pad->bRightTrigger) / 255.0f;

        *controller = newController;
    }
}

internal bool Win32InitWindow(Win32ScreenBuffer *screen, HINSTANCE instance) {
    Win32ResizeDIBSection(screen, 1280, 720);

    WNDCLASSA windowClass = {
        .style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc   = Win32MainWindowCallback,
        .hInstance     = instance,
        .hIcon         = 0,
        .lpszClassName = "HandmadeHeroClassName",
    };

    if (!RegisterClassA(&windowClass)) {
        OutputDebugStringA("[ERROR] Couldn't register window class");
        return false;
    }

    screen->window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
    if (!screen->window) {
        OutputDebugStringA("[ERROR] Couldn't create window");
        return false;
    }
    screen->deviceContext = GetDC(screen->window);

    return true;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
    Win32InitWindow(&Win32Screen, instance);
    Win32InitSound(&Win32Sound);

    // <Performance>
    LARGE_INTEGER perfCountFrequencyResult = {};
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    i64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

    LARGE_INTEGER lastCounter = {};
    QueryPerformanceCounter(&lastCounter);

    u64 lastCycleCount = __rdtsc();
    // </Performance>

    while (Running) {
        // PeekMessageA(..., 0, 0, 0, ..) gets messages from all windows (hWnds) in the application,
        // without blocking when there's no message.
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                Running = false;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        Win32ProcessXInputControllers(&Win32Input);
        UpdateAndRender(&Win32Input, &Win32Screen, &Win32Sound);

        v2i dim = GetWindowDimension(Win32Screen.window);
        Win32DisplayBuffer(Win32Screen, Win32Screen.deviceContext, dim.x, dim.y);
        ReleaseDC(Win32Screen.window, Win32Screen.deviceContext);

        // <Performance>
        u64 endCycleCount = __rdtsc();
        u64 cyclesElapsed = endCycleCount - lastCycleCount;

        LARGE_INTEGER endCounter;
        QueryPerformanceCounter(&endCounter);
        i64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;

        f64 msPerFrame         = (1000.0 * f64(counterElapsed)) / f64(perfCountFrequency);
        f64 framesPerSec       = f64(perfCountFrequency) / f64(counterElapsed);
        f64 megaCyclesPerFrame = f64(cyclesElapsed) / (1000.0 * 1000.0);

        char buf[128];
        WIN32_CHECK(StringCbPrintfA(buf, sizeof(buf), "%.2f ms/f, %.2f fps, %.2f mc/f\n",
                                    msPerFrame, framesPerSec, megaCyclesPerFrame));
        OutputDebugStringA(buf);

        lastCounter    = endCounter;
        lastCycleCount = endCycleCount;
        // </Performance>
    }

    return 0;
}