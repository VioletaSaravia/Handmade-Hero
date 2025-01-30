#include <Windows.h>

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case WM_SIZE:  // Window size changes
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY:  // Window destroyed
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;

        case WM_CLOSE:  // Window 'X' clicked
        {
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC         deviceContext = BeginPaint(window, &paint);

            PatBlt(deviceContext, paint.rcPaint.left, paint.rcPaint.top,
                   paint.rcPaint.right - paint.rcPaint.left,
                   paint.rcPaint.bottom - paint.rcPaint.top, WHITENESS);

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
        .style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,  // TODO(violeta): Are these necessary?
        .lpfnWndProc   = MainWindowCallback,
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

    for (;;) {
        MSG  message;
        BOOL messageResult = GetMessageA(&message, 0, 0, 0);
        if (messageResult <= 0) break;

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return (0);
}