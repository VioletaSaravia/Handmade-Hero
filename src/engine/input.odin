package engine


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