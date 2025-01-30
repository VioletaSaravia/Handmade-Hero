#include <Windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO(violeta): Refactor this globals out.
global_variable bool       Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void      *BitmapMemory;
global_variable HBITMAP    BitmapHandle;
global_variable HDC        BitmapDeviceContext;

// DIB = Device-independent bitmap
internal void Win32ResizeDIBSection(int width, int height) {
    if (BitmapHandle) {
        DeleteObject(BitmapHandle);
    }

    if (!BitmapDeviceContext) {
        BitmapDeviceContext = CreateCompatibleDC(0);
    }

    BitmapInfo = {.bmiHeader = {.biSize          = sizeof(BitmapInfo.bmiHeader),
                                .biWidth         = width,
                                .biHeight        = height,
                                .biPlanes        = 1,
                                .biBitCount      = 32,  // not 24 so we get DWORD alignment
                                .biCompression   = BI_RGB,
                                .biSizeImage     = 0,
                                .biXPelsPerMeter = 0,  // TODO(violeta): can this be 0?
                                .biYPelsPerMeter = 0,
                                .biClrUsed       = 0,
                                .biClrImportant  = 0}};

    BitmapHandle =
        CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

internal void Win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height) {
    StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, &BitmapMemory,
                  &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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

            Win32UpdateWindow(deviceContext, paint.rcPaint.left, paint.rcPaint.top,
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
    MessageBoxA(0, "Hello world!", "Handmade Hero", MB_OK | MB_ICONINFORMATION);

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

    HWND windowHandle = CreateWindowExA(
        0, windowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

    if (!windowHandle) {
        // TODO log
    }

    Running = true;
    while (Running) {
        MSG message;

        // GetMessage(..., 0, 0, 0) gets messages from all windows (hWnds) in the application
        BOOL messageResult = GetMessageA(&message, 0, 0, 0);
        if (messageResult <= 0) break;

        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return (0);
}