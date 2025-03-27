package engine

import "core:fmt"
import win "core:sys/windows"
import gl "vendor:OpenGL"

ButtonState :: enum u8 {
	Released,
	JustReleased,
	Pressed,
	JustPressed,
}

GamepadButton :: enum u8 {
	Up,
	Down,
	Left,
	Right,
	A,
	B,
	X,
	Y,
	ShL,
	ShR,
	Start,
	Back,
	ThumbL,
	ThumbR,
	COUNT,
}

ControllerState :: struct {
	connected, not_analog:                              bool,
	buttons:                                            [GamepadButton.COUNT]ButtonState,
	trig_l_start, trig_l_end, trig_r_start, trig_r_end: f32,
	l_start, l_end, r_start, r_end:                     v2,
	min_x, min_y, max_x, max_y:                         f32,
}

KEY_COUNT :: 256
CONTROLLER_COUNT :: 4

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
	F1       = 0x70,
	F2       = 0x71,
	F3       = 0x72,
	F4       = 0x73,
	F5       = 0x74,
	F6       = 0x75,
	F7       = 0x76,
	F8       = 0x77,
	F9       = 0x78,
	F10      = 0x79,
	F11      = 0x7A,
	F12      = 0x7B,
}

MouseState :: struct {
	left, right, middle: ButtonState,
	wheel:               i16,
}

InputBuffer :: struct {
	gamepads: [CONTROLLER_COUNT]ControllerState,
	keyboard: [KEY_COUNT]ButtonState,
	mouse:    MouseState,
}

MainWindowCallback :: proc "std" (
	window: win.HWND,
	msg: u32,
	w_param: win.WPARAM,
	l_param: win.LPARAM,
) -> (
	result: win.LRESULT,
) {
	switch msg {
	case win.WM_SETCURSOR:
		if win.LOWORD(l_param) == win.HTCLIENT {
			win.SetCursor(win.LoadCursorA(nil, win.IDC_ARROW))
			result = 1
		} else {
			result = win.DefWindowProcA(window, msg, w_param, l_param)
		}

	case win.WM_SIZE:

	case win.WM_DESTROY:
		win.OutputDebugStringA("Window forcefully closed")
		Mem.Running = false
		win.PostQuitMessage(0)

	case win.WM_CLOSE:
		Mem.Running = false

	case win.WM_MOUSEWHEEL:
		Mem.Input.mouse.wheel = auto_cast win.HIWORD(w_param)

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

	return
}

ProcessKeyboard :: proc(buffer: ^InputBuffer) {
	for &i in buffer.keyboard {
		if i == .JustReleased do i = .Released
	}

	message: win.MSG
	for (win.PeekMessageA(
			    &message,
			    nil,
			    0,
			    0,
			    1,
			    /* PM_REMOVE ? */
		    )) {
		if message.message == win.WM_QUIT do Mem.Running = false
		win.TranslateMessage(&message)
		win.DispatchMessageW(&message)
	}
}

MAX_CONTROLLERS ::
	win.XUSER_MAX_COUNT when CONTROLLER_COUNT > win.XUSER_MAX_COUNT else CONTROLLER_COUNT

ProcessControllers :: proc(buffer: ^InputBuffer) {
	for i in 0 ..< CONTROLLER_COUNT {
		new_controller: ControllerState
		controller := &buffer.gamepads[i]
		xinput_state: win.XINPUT_STATE
		if win.XInputGetState(win.XUSER(i), &xinput_state) != win.System_Error.SUCCESS {
			if controller.connected {
				// LOG
				controller.connected = false
			}
			continue
		}

		if !controller.connected {
			// LOG
			controller.connected = true
		}

		gamepad := &xinput_state.Gamepad

		for j in GamepadButton {
			switch controller.buttons[j] {
			case .Pressed, .JustPressed:
				new_controller.buttons[j] = .DPAD_UP in gamepad.wButtons ? .Pressed : .JustReleased
			case .Released, .JustReleased:
				new_controller.buttons[j] = .DPAD_UP in gamepad.wButtons ? .Pressed : .JustReleased
			}
		}

		new_controller.l_start = controller.l_end
		new_controller.r_start = controller.r_end

		stick_lx := f32(gamepad.sThumbLX) / (gamepad.sThumbLX < 0 ? -32768.0 : 32767.0)
		stick_ly := f32(gamepad.sThumbLY) / (gamepad.sThumbLY < 0 ? -32768.0 : 32767.0)
		stick_rx := f32(gamepad.sThumbRX) / (gamepad.sThumbRX < 0 ? -32768.0 : 32767.0)
		stick_ry := f32(gamepad.sThumbRY) / (gamepad.sThumbRY < 0 ? -32768.0 : 32767.0)

		new_controller.l_end = {stick_lx, stick_ly}
		new_controller.r_end = {stick_rx, stick_ry}

		new_controller.trig_l_start = controller.trig_l_end
		new_controller.trig_r_start = controller.trig_r_end
		new_controller.trig_l_end = f32(gamepad.bLeftTrigger) / 255.0
		new_controller.trig_r_end = f32(gamepad.bRightTrigger) / 255.0

		controller^ = new_controller
	}
}

ProcessMouse :: proc(buffer: ^InputBuffer) {
	// La rueda del mouse se maneja en MainWindowCallback

	if (win.GetAsyncKeyState(win.VK_LBUTTON) & -1) !=  /* ? */0 {
		buffer.mouse.left = buffer.mouse.left == .JustPressed ? .Pressed : .JustPressed
	} else {
		buffer.mouse.left = buffer.mouse.left == .JustReleased ? .Released : .JustReleased
	}
	if (win.GetAsyncKeyState(win.VK_RBUTTON) & -1) !=  /* ? */0 {
		buffer.mouse.right = buffer.mouse.right == .JustPressed ? .Pressed : .JustPressed
	} else {
		buffer.mouse.right = buffer.mouse.right == .JustReleased ? .Released : .JustReleased
	}
	if (win.GetAsyncKeyState(win.VK_MBUTTON) & -1) !=  /* ? */0 {
		buffer.mouse.middle = buffer.mouse.middle == .JustPressed ? .Pressed : .JustPressed
	} else {
		buffer.mouse.middle = buffer.mouse.middle == .JustReleased ? .Released : .JustReleased
	}
}
