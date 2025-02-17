#include <dsound.h>
#include <math.h>
#include <stdint.h>
#include <strsafe.h>
#include <Windows.h>
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

typedef struct {
    BITMAPINFO          Info;
    GameOffscreenBuffer buffer;
} Win32OffscreenBuffer;

global_variable bool                 Running = true;
global_variable Win32OffscreenBuffer Win32Buffer;
global_variable LPDIRECTSOUNDBUFFER  SecondarySoundBuffer;

#ifndef DEBUG
#define WIN32_CHECK(func) (func)
#else
#define WIN32_CHECK(func)                                                  \
    if (FAILED(func)) {                                                    \
        char buf[32];                                                      \
        StringCbPrintfA(buf, sizeof(buf), "[ERROR] %d\n", GetLastError()); \
        OutputDebugStringA(buf);                                           \
    }
#endif

internal void Win32InitDSound(HWND window, u32 bufferSize, u32 samplesPerSecond) {
    LPDIRECTSOUND directSound = {};

    WIN32_CHECK(DirectSoundCreate(0, &directSound, 0));

    WIN32_CHECK(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY));

    LPDIRECTSOUNDBUFFER primaryBuffer     = {};
    DSBUFFERDESC        bufferDescription = {
               .dwSize  = sizeof(DSBUFFERDESC),
               .dwFlags = DSBCAPS_PRIMARYBUFFER,
    };

    WIN32_CHECK(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0));

    WAVEFORMATEX waveFormat = {
        .wFormatTag      = WAVE_FORMAT_PCM,
        .nChannels       = 2,
        .nSamplesPerSec  = samplesPerSecond,
        .nAvgBytesPerSec = samplesPerSecond * ((2 * 16) / 8),
        .nBlockAlign     = (2 * 16) / 8,
        .wBitsPerSample  = 16,
        .cbSize          = 0,
    };

    WIN32_CHECK(primaryBuffer->SetFormat(&waveFormat));

    // Secondary buffer
    DSBUFFERDESC bufferDescription2 = {
        .dwSize        = sizeof(DSBUFFERDESC),
        .dwFlags       = DSBCAPS_GLOBALFOCUS,  // TODO(violeta): Global focus y/n?
        .dwBufferBytes = bufferSize,
        .lpwfxFormat   = &waveFormat,  // TODO primary too?
    };

    WIN32_CHECK(directSound->CreateSoundBuffer(&bufferDescription2, &SecondarySoundBuffer, 0));
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
internal void Win32ResizeDIBSection(Win32OffscreenBuffer *win32Buffer, int width, int height) {
    if (win32Buffer->buffer.Memory) {
        VirtualFree(win32Buffer->buffer.Memory, 0, MEM_RELEASE);
    }

    win32Buffer->buffer.Width         = width;
    win32Buffer->buffer.Height        = height;
    win32Buffer->buffer.BytesPerPixel = 4;

    win32Buffer->Info = {
        .bmiHeader = {.biSize  = sizeof(win32Buffer->Info.bmiHeader),
                      .biWidth = win32Buffer->buffer.Width,
                      // Negative height tells Windows to treat the window's y axis as top-down
                      .biHeight        = -win32Buffer->buffer.Height,
                      .biPlanes        = 1,
                      .biBitCount      = 32,  // 4 byte align
                      .biCompression   = BI_RGB,
                      .biSizeImage     = 0,
                      .biXPelsPerMeter = 0,
                      .biYPelsPerMeter = 0,
                      .biClrUsed       = 0,
                      .biClrImportant  = 0}};

    u64 bitmapMemorySize = u64(win32Buffer->buffer.Width * win32Buffer->buffer.Height) *
                           win32Buffer->buffer.BytesPerPixel;
    win32Buffer->buffer.Memory =
        VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal void Win32DisplayBuffer(Win32OffscreenBuffer buffer, HDC deviceContext, int windowWidth,
                                 int windowHeight) {
    // TODO(violeta): Aspect ratio correction and stretch modes
    if (!StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer.buffer.Width,
                       buffer.buffer.Height, buffer.buffer.Memory, &buffer.Info, DIB_RGB_COLORS,
                       SRCCOPY)) {
        // TODO: log
    };
}

#define SAMPLES_PER_SEC 48 * 1000

// ----- Test variables
i32 toneHz   = 256;
i32 wvPeriod = SAMPLES_PER_SEC / toneHz;
i32 volume   = 4000;

i32 xOffset = 0;
i32 yOffset = 0;
// -----

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
            Win32DisplayBuffer(Win32Buffer, deviceContext, dim.Width, dim.Height);

            EndPaint(window, &paint);
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
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
    Win32ResizeDIBSection(&Win32Buffer, 1280, 720);

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

    u32 bytesPerSample     = sizeof(i16) * 2;
    u32 bufferSize         = SAMPLES_PER_SEC * bytesPerSample;
    u32 runningSampleIndex = 0;
    Win32InitDSound(window, bufferSize, SAMPLES_PER_SEC);
    bool firstSoundLoop = true;  // TODO(violeta): Doesn't seem to do anything?

    LARGE_INTEGER perfCountFrequencyResult = {};
    WIN32_CHECK(QueryPerformanceFrequency(&perfCountFrequencyResult));
    i64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

    LARGE_INTEGER lastCounter = {};
    WIN32_CHECK(QueryPerformanceCounter(&lastCounter));

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

        UpdateAndRender(&Win32Buffer.buffer);

        // ===== SOUND
        DWORD playCursor, writeCursor;
        WIN32_CHECK(SecondarySoundBuffer->GetCurrentPosition(&playCursor, &writeCursor));

        DWORD byteToLock = (runningSampleIndex * bytesPerSample) % bufferSize;

        DWORD bytesToWrite = byteToLock > playCursor ? bufferSize - byteToLock + playCursor
                                                     : playCursor - byteToLock;

        void *region1, *region2;
        DWORD region1Size, region2Size;
        WIN32_CHECK(SecondarySoundBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size,
                                               &region2, &region2Size, 0));

        // TODO: assert region1/2Size are valid
        i16  *sampleOut          = (i16 *)region1;
        DWORD region1SampleCount = region1Size / bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++) {
            // sampleValue = ((runningSampleIndex / (wvPeriod / 2)) % 2) ? volume : -volume;
            f32 t           = TAU * f32(runningSampleIndex) / f32(wvPeriod);
            f32 sineValue   = sinf(t);
            i16 sampleValue = i16(sineValue * volume);

            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            runningSampleIndex++;
        }

        sampleOut                = (i16 *)region2;
        DWORD region2SampleCount = region2Size / bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++) {
            // i16 sampleValue = ((runningSampleIndex / (wvPeriod / 2)) % 2) ? volume : -volume;
            f32 t           = TAU * f32(runningSampleIndex) / f32(wvPeriod);
            f32 sineValue   = sinf(t);
            i16 sampleValue = i16(sineValue * volume);

            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            runningSampleIndex++;
        }

        SecondarySoundBuffer->Unlock(region1, region1Size, region2, region2Size);

        if (firstSoundLoop) {
            SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
            firstSoundLoop = false;
        }
        // =====

        auto dim = GetWindowDimension(window);
        Win32DisplayBuffer(Win32Buffer, deviceContext, dim.Width, dim.Height);
        ReleaseDC(window, deviceContext);

        // TEST vars
        ++xOffset;
        ++yOffset;

        // ##### PERFORMANCE
        u64 endCycleCount = __rdtsc();
        u64 cyclesElapsed = endCycleCount - lastCycleCount;

        LARGE_INTEGER endCounter;
        WIN32_CHECK(QueryPerformanceCounter(&endCounter));
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
        // #####
    }

    return 0;
}