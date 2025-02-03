#include <dsound.h>
#include <math.h>
#include <stdint.h>
#include <Windows.h>
#include <Xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef double f64;
typedef float  f32;

#define PI 3.14159265359f
#define TAU 6.283185307179586f

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef int32_t b32;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef unsigned char u8;

typedef double f64;
typedef float  f32;

struct Win32OffscreenBuffer {
    void      *Memory;
    int        Width, Height;
    int        BytesPerPixel;
    BITMAPINFO Info;
};

global_variable bool                 Running = true;
global_variable Win32OffscreenBuffer BackBuffer;
global_variable LPDIRECTSOUNDBUFFER  SecondarySoundBuffer;

internal void Win32InitDSound(HWND window, u32 bufferSize, u32 samplesPerSecond) {
    LPDIRECTSOUND directSound;
    // TODO(violeta): Why even use this macro?
    if (!SUCCEEDED(DirectSoundCreate(0, &directSound, 0))) {
        return;
    }

    if (!SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
        return;
    };

    LPDIRECTSOUNDBUFFER primaryBuffer     = {};
    DSBUFFERDESC        bufferDescription = {
               .dwSize  = sizeof(DSBUFFERDESC),
               .dwFlags = DSBCAPS_PRIMARYBUFFER,
    };
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) {
        return;
    };

    WAVEFORMATEX waveFormat = {
        .wFormatTag      = WAVE_FORMAT_PCM,
        .nChannels       = 2,
        .nSamplesPerSec  = samplesPerSecond,
        .nAvgBytesPerSec = samplesPerSecond * ((2 * 16) / 8),
        .nBlockAlign     = (2 * 16) / 8,
        .wBitsPerSample  = 16,
        .cbSize          = 0,
    };

    if (!SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) {
        return;
    };

    // Secondary buffer
    DSBUFFERDESC bufferDescription2 = {
        .dwSize        = sizeof(DSBUFFERDESC),
        .dwFlags       = DSBCAPS_GLOBALFOCUS,  // TODO(violeta): Global focus y/n?
        .dwBufferBytes = bufferSize,
        .lpwfxFormat   = &waveFormat,  // TODO primary too?
    };
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription2, &SecondarySoundBuffer, 0))) {
        return;
    };
}

struct Win32WindowDimension {
    int Width, Height;
};

Win32WindowDimension GetWindowDimension(HWND window) {
    RECT clientRect;  // Rect of "client" (drawable area)
    GetClientRect(window, &clientRect);
    return {clientRect.right - clientRect.left, clientRect.bottom - clientRect.top};
}

internal void RenderWeirdGradient(Win32OffscreenBuffer *buffer, int xOffset, int yOffset) {
    int width  = buffer->Width;
    int height = buffer->Height;

    int stride = width * buffer->BytesPerPixel;
    u8 *row    = (u8 *)buffer->Memory;
    for (int y = 0; y < height; y++) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < width; x++) {
            /*
            Pixel: BB GG RR -- (4 bytes)
                   0  1  2  3
            */
            u8 blue  = u8(x + xOffset);
            u8 green = u8(y + yOffset);
            u8 red   = 0;

            *pixel++ = u32(blue | (green << 8) | (red << 16));
        }

        row += stride;
    }
}

// DIB = Device-independent bitmap
internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height) {
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

internal void Win32DisplayBuffer(Win32OffscreenBuffer buffer, HDC deviceContext, int windowWidth,
                                 int windowHeight) {
    // TODO(violeta): Aspect ratio correction and stretch modes
    if (!StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer.Width,
                       buffer.Height, buffer.Memory, &buffer.Info, DIB_RGB_COLORS, SRCCOPY)) {
        // TODO: log
    };
}

// ----- Test variables
i32 toneHz   = 256;
i32 wvPeriod = (48 * 1000) /* samplesPerSecond */ / toneHz;
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
            // TODO(violeta): Add user message
            Running = false;
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC         deviceContext = BeginPaint(window, &paint);

            auto dim = GetWindowDimension(window);
            Win32DisplayBuffer(BackBuffer, deviceContext, dim.Width, dim.Height);

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
    Win32ResizeDIBSection(&BackBuffer, 1280, 720);

    WNDCLASSA windowClass = {
        .style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc   = Win32MainWindowCallback,
        .hInstance     = instance,
        .hIcon         = 0,
        .lpszClassName = "HandmadeHeroClassName",
    };

    if (!RegisterClassA(&windowClass)) {
        // TODO log
    }

    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

    if (!window) {
        // TODO log
    }
    HDC deviceContext = GetDC(window);

    i32 samplesPerSecond   = 48 * 1000;
    u32 bytesPerSample     = sizeof(i16) * 2;
    u32 bufferSize         = samplesPerSecond * bytesPerSample;
    u32 runningSampleIndex = 0;
    Win32InitDSound(window, bufferSize, samplesPerSecond);
    bool firstSoundLoop = true;  // TODO(violeta): Doesn't seem to do anything?

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

        RenderWeirdGradient(&BackBuffer, xOffset, yOffset);

        // ===== SOUND
        DWORD playCursor, writeCursor;
        if (!SUCCEEDED(SecondarySoundBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
            // TODO: Log
        }

        DWORD byteToLock = (runningSampleIndex * bytesPerSample) % bufferSize;

        // TODO: Change to a lower latency offset from playcursor when we implement sfx.
        DWORD bytesToWrite = byteToLock > playCursor ? bufferSize - byteToLock + playCursor
                                                     : playCursor - byteToLock;

        void *region1, *region2;
        DWORD region1Size, region2Size;
        if (!SUCCEEDED(SecondarySoundBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size,
                                                  &region2, &region2Size, 0))) {
            // TODO: Log
        };

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
        Win32DisplayBuffer(BackBuffer, deviceContext, dim.Width, dim.Height);
        ReleaseDC(window, deviceContext);

        ++xOffset;
        ++yOffset;
    }

    return 0;
}