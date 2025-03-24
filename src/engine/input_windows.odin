package engine

import win "core:sys/windows"

// TODO
Key :: enum u8 {
	Ret      = 0x0D,
	Shift    = 0x10,
	Ctrl     = 0x11,
	Esc      = 0x1B,
	Space    = 0x20,
	KeyLeft  = 0x25,
	KeyUp    = 0x26,
	KeyRight = 0x27,
	KeyDown  = 0x28,
}

InputBuffer :: struct {
	pads:     [CONTROLLER_COUNT]ControllerState,
	keyboard: [KEY_COUNT]ButtonState,
}

MainWindowCallback :: proc "std" (
	window: win.HWND,
	msg: u32,
	w_param: win.WPARAM,
	l_param: win.LPARAM,
) -> win.LRESULT {
	result: win.LRESULT = 0

	switch msg {
	case win.WM_SIZE:

	case win.WM_DESTROY:
		// LOG
		Running = false

	case win.WM_CLOSE:
		Running = false

	case win.WM_PAINT:
		paint: win.PAINTSTRUCT
		device_ctx := win.BeginPaint(window, &paint)
		win.EndPaint(window, &paint)

	case win.WM_ACTIVATEAPP:

	case win.WM_SYSKEYDOWN, win.WM_SYSKEYUP, win.WM_KEYDOWN, win.WM_KEYUP:
		was_down := l_param & (1 << 30)
		is_down := l_param & (1 << 31) == 0

	case:
		result = win.DefWindowProcA(window, msg, w_param, l_param)
	}

	return result
}

ProcessKeyboard :: proc() {}

ProcessControllers :: proc() {}
