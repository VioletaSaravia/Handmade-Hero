#include <stdint.h>
#include <Windows.h>

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

global_variable bool       Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void      *BitmapMemory;
global_variable int        BitmapWidth, BitmapHeight;
global_variable int        BytesPerPixel = 4;

internal void RenderWeirdGradient(int xOffset, int yOffset) {
    int width  = BitmapWidth;
    int height = BitmapHeight;

    int pitch = width * BytesPerPixel;
    u8 *row   = (u8 *)BitmapMemory;
    for (int y = 0; y < height; y++) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < width; x++) {
            /*      0  1  2  3
            Pixel: BB GG RR --
            */
            u8 blue  = x + xOffset;
            u8 green = y + yOffset;
            u8 red   = 0;

            *pixel++ = blue | (green << 8) | (red << 16);
        }

        row += pitch;
    }
}

// DIB = Device-independent bitmap
internal void Win32ResizeDIBSection(int width, int height) {
    if (BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth  = width;
    BitmapHeight = height;

    BitmapInfo = {.bmiHeader = {.biSize          = sizeof(BitmapInfo.bmiHeader),
                                .biWidth         = BitmapWidth,
                                .biHeight        = -BitmapHeight,
                                .biPlanes        = 1,
                                .biBitCount      = 32,  // 4 byte align
                                .biCompression   = BI_RGB,
                                .biSizeImage     = 0,
                                .biXPelsPerMeter = 0,  // TODO(violeta): can this be 0?
                                .biYPelsPerMeter = 0,
                                .biClrUsed       = 0,
                                .biClrImportant  = 0}};

    int bitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
    BitmapMemory         = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC deviceContext, RECT *clientRect, int x, int y, int width,
                                int height) {
    int windowWidth  = clientRect->right - clientRect->left;
    int windowHeight = clientRect->bottom - clientRect->top;

    int ok = StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight, 0, 0, BitmapWidth,
                           BitmapHeight, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE:  // Window size changes
        {
            RECT clientRect;  // Rect of "client" (drawable area)
            GetClientRect(window, &clientRect);
            int width  = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            Win32ResizeDIBSection(width, height);
            OutputDebugStringA("WM_SIZE\n");
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

            RECT clientRect;  // Rect of "client" (drawable area)
            GetClientRect(window, &clientRect);
            Win32UpdateWindow(deviceContext, &clientRect, paint.rcPaint.left, paint.rcPaint.top,
                              paint.rcPaint.right - paint.rcPaint.left,
                              paint.rcPaint.bottom - paint.rcPaint.top);

            EndPaint(window, &paint);
        } break;

        case WM_ACTIVATEAPP:  // Window selected by user
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }

    return (result);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
    // MessageBoxA(0, "Hello world!", "Handmade Hero", MB_OK | MB_ICONINFORMATION);

    WNDCLASSA windowClass = {
        // TODO(violeta): Are these necessary?
        .style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
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

    int xOffset = 0;
    int yOffset = 0;

    Running = true;
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

        RenderWeirdGradient(xOffset, yOffset);

        HDC  deviceContext = GetDC(window);
        RECT clientRect;
        GetClientRect(window, &clientRect);
        int windowWidth  = clientRect.right - clientRect.left;
        int windowHeight = clientRect.bottom - clientRect.top;
        Win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
        ReleaseDC(window, deviceContext);

        ++xOffset;
    }

    return (0);
}