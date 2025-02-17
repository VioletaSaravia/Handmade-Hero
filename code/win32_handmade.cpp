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
    BITMAPINFO Info;
};
global_variable Win32ScreenBuffer Win32Screen;

struct Win32SoundBuffer : SoundBuffer {
    IXAudio2               *xAudio2;
    IXAudio2MasteringVoice *xAudio2MasteringVoice;
    IXAudio2SourceVoice    *xAudio2SourceVoice;
};
global_variable Win32SoundBuffer Win32Sound;

global_variable bool Running = true;

#ifndef DEBUG
#define WIN32_ASSERT(func) (func)
#else
#define WIN32_ASSERT(func)                                                       \
    if (FAILED(func)) {                                                          \
        char buf[32];                                                            \
        StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %d\n", GetLastError()); \
        OutputDebugStringA(buf);                                                 \
    }
#endif

constexpr u16 BITSPERSSAMPLE           = 16;
constexpr u32 SAMPLESPERSEC            = 44100;
constexpr f64 CYCLESPERSEC             = 220.0;
constexpr f64 VOLUME                   = 0.5;
constexpr u16 AUDIOBUFFERSIZEINCYCLES  = 10;
constexpr u32 SAMPLESPERCYCLE          = u32(SAMPLESPERSEC / CYCLESPERSEC);
constexpr u32 AUDIOBUFFERSIZEINSAMPLES = SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES;
constexpr u32 AUDIOBUFFERSIZEINBYTES   = AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE / 8;

internal void Win32InitSound(Win32SoundBuffer *sound) {
    WIN32_ASSERT(CoInitializeEx(0, COINIT_MULTITHREADED));

    WIN32_ASSERT(XAudio2Create(&sound->xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR));
    WIN32_ASSERT(sound->xAudio2->CreateMasteringVoice(&sound->xAudio2MasteringVoice));

    sound->samplesPerSec = SAMPLESPERSEC;
    sound->sampleCount   = AUDIOBUFFERSIZEINSAMPLES;
    sound->byteCount     = AUDIOBUFFERSIZEINBYTES;
    sound->sampleOut     = (u8 *)malloc(AUDIOBUFFERSIZEINBYTES);

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

    for (u32 i = 0; i < AUDIOBUFFERSIZEINBYTES;) {
        sound->sampleOut[i++] = 0;
        sound->sampleOut[i++] = 0;
    }

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

    WIN32_ASSERT(sound->xAudio2SourceVoice->SubmitSourceBuffer(&xAudio2Buffer));
    WIN32_ASSERT(sound->xAudio2SourceVoice->Start(0));
}

struct Win32WindowDimension {
    int Width, Height;
};
Win32WindowDimension GetWindowDimension(HWND window) {
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
    buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
            Win32DisplayBuffer(Win32Screen, deviceContext, dim.Width, dim.Height);

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

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
    Win32ResizeDIBSection(&Win32Screen, 1280, 720);

    WNDCLASSA windowClass = {
        .style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc   = Win32MainWindowCallback,
        .hInstance     = instance,
        .hIcon         = 0,
        .lpszClassName = "HandmadeHeroClassName",
    };

    if (!RegisterClassA(&windowClass)) {
        OutputDebugStringA("[ERROR] Couldn't register window class");
        return -1;
    }

    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
    if (!window) {
        OutputDebugStringA("[ERROR] Couldn't create window");
        return -1;
    }
    HDC deviceContext = GetDC(window);

    Win32InitSound(&Win32Sound);

    LARGE_INTEGER perfCountFrequencyResult = {};
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    i64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

    LARGE_INTEGER lastCounter = {};
    QueryPerformanceCounter(&lastCounter);

    u64 lastCycleCount = __rdtsc();

    while (Running) {
        MSG message;

        // PeekMessageA(..., 0, 0, 0, ..) gets messages from all windows (hWnds) in the application,
        // without blocking when there's no message.
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                Running = false;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
            XINPUT_STATE controllerState = {};
            if (XInputGetState(controllerIndex, &controllerState) != ERROR_SUCCESS) {
                // TODO(violeta): Controller not connected
                continue;
            }

            auto pad = &controllerState.Gamepad;

            bool up        = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool down      = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool left      = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool right     = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool start     = (pad->wButtons & XINPUT_GAMEPAD_START);
            bool back      = (pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool shoulderL = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool shoulderR = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool btnA      = (pad->wButtons & XINPUT_GAMEPAD_A);
            bool btnB      = (pad->wButtons & XINPUT_GAMEPAD_B);
            bool btnX      = (pad->wButtons & XINPUT_GAMEPAD_X);
            bool btnY      = (pad->wButtons & XINPUT_GAMEPAD_Y);

            i16 stickLX = pad->sThumbLX;
            i16 stickLY = pad->sThumbLY;
            i16 stickRX = pad->sThumbRX;
            i16 stickRY = pad->sThumbRY;
        }

        UpdateAndRender(&Win32Screen, &Win32Sound);

        auto dim = GetWindowDimension(window);
        Win32DisplayBuffer(Win32Screen, deviceContext, dim.Width, dim.Height);
        ReleaseDC(window, deviceContext);

        // ##### PERFORMANCE
        u64 endCycleCount = __rdtsc();
        u64 cyclesElapsed = endCycleCount - lastCycleCount;

        LARGE_INTEGER endCounter;
        QueryPerformanceCounter(&endCounter);
        i64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;

        f64 msPerFrame         = (1000.0 * f64(counterElapsed)) / f64(perfCountFrequency);
        f64 framesPerSec       = f64(perfCountFrequency) / f64(counterElapsed);
        f64 megaCyclesPerFrame = f64(cyclesElapsed) / (1000.0 * 1000.0);

        char buf[128];
        WIN32_ASSERT(StringCbPrintfA(buf, sizeof(buf), "%.2f ms/f, %.2f fps, %.2f mc/f\n",
                                     msPerFrame, framesPerSec, megaCyclesPerFrame));
        OutputDebugStringA(buf);

        lastCounter    = endCounter;
        lastCycleCount = endCycleCount;
        // #####
    }

    return 0;
}