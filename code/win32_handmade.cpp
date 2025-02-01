#include <stdint.h>
#include <Windows.h>
#include <Xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef unsigned char u8;

typedef double f64;
typedef float  f32;

global_variable bool Running = true;

struct Win32OffscreenBuffer {
    BITMAPINFO Info;
    void      *Memory;
    int        Width, Height;
    int        BytesPerPixel;
};

global_variable Win32OffscreenBuffer BackBuffer;

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
            u8 blue  = x + xOffset;
            u8 green = y + yOffset;
            u8 red   = 0;

            *pixel++ = blue | (green << 8) | (red << 16);
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
                      // Negative height tells Sindows to treat the window's y axis as top-down
                      .biHeight        = -buffer->Height,
                      .biPlanes        = 1,
                      .biBitCount      = 32,  // 4 byte align
                      .biCompression   = BI_RGB,
                      .biSizeImage     = 0,
                      .biXPelsPerMeter = 0,
                      .biYPelsPerMeter = 0,
                      .biClrUsed       = 0,
                      .biClrImportant  = 0}};

    int bitmapMemorySize = (buffer->Width * buffer->Height) * buffer->BytesPerPixel;
    buffer->Memory       = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBuffer(Win32OffscreenBuffer buffer, HDC deviceContext, int windowWidth,
                                 int windowHeight) {
    // TODO(violeta): Aspect ratio correction and stretch modes
    if (!StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, buffer.Width,
                       buffer.Height, buffer.Memory, &buffer.Info, DIB_RGB_COLORS, SRCCOPY)) {
        // TODO: log
    };
}

// TODO(violeta): Dynamic loading of xinput. Not necessary in 2025?
// DWORD XInputGetState(DWORD dwUserIndex, XINPUT_STATE *pState);
// DWORD XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE:  // Window size changes
        {
        } break;

        case WM_DESTROY:  // Window destroyed
        {
            // TODO(violeta): Handle as error - recreate window
            Running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;

        case WM_CLOSE:  // Window 'X' clicked
        {
            // TODO(violeta): Add user message
            Running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC         deviceContext = BeginPaint(window, &paint);

            auto dim = GetWindowDimension(window);
            Win32DisplayBuffer(BackBuffer, deviceContext, dim.Width, dim.Height);

            EndPaint(window, &paint);
        } break;

        case WM_ACTIVATEAPP:  // Window selected by user
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            u32  vKCode  = wParam;
            bool wasDown = (lParam & (1 << 30)) != 0;
            bool isDown  = (lParam & (1 << 31)) == 0;

            switch (vKCode) {
                case 'W' | VK_UP:
                    break;

                case 'A' | VK_LEFT:
                    break;

                case 'S' | VK_DOWN:
                    break;

                case 'D' | VK_RIGHT:
                    break;

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

    return (result);
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

    int xOffset = 0;
    int yOffset = 0;

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

        auto dim = GetWindowDimension(window);
        Win32DisplayBuffer(BackBuffer, deviceContext, dim.Width, dim.Height);
        ReleaseDC(window, deviceContext);

        ++xOffset;
        ++yOffset;
    }

    return (0);
}