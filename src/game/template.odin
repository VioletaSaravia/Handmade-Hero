package game

import e "../engine"
import fmt "core:fmt"

s: ^GameState
GameState :: struct {
	player_pos: e.v2,
	imgs:       [2]e.Image,
}

@(export)
GameSetup :: proc() {
	e.Settings(
		{
			name = "Test",
			version = "0.1",
			resolution = {800, 600},
			fullscreen = false,
			memory = size_of(GameState),
		},
	)
	e.Mem.LoadData = proc() {
		s = auto_cast e.GetUserMemory(0)
	}
}

@(export)
GameInit :: proc() {
	s.imgs[0], _ = e.LoadImage("door.bmp")
	s.imgs[1], _ = e.LoadImage("ship.bmp")
	e.Audio().sounds[0] = e.LoadSound("kick.wav")
	e.Audio().sounds[1] = e.LoadSound("ambience.mp3", .looping)
}

@(export)
GameUpdate :: proc() {
	for k in 'A' ..= 'Z' {
		if e.Input().keys[k] == .JustPressed do fmt.println("Pressed the key", k)
	}

	fmt.println(
		"Mouse L/R/M/W:",
		e.Input().mouse.left,
		e.Input().mouse.right,
		e.Input().mouse.middle,
		e.Input().mouse.wheel,
	)

	if e.Input().keys['P'] == .JustPressed do e.PlaySound(&e.Audio().sounds[0])
	if e.Input().keys['V'] >= .Pressed do e.Audio().sounds[0].pan -= (2 * e.Delta())
	if e.Input().keys['B'] >= .Pressed do e.Audio().sounds[0].pan += (2 * e.Delta())

	if e.Input().keys['O'] == .JustPressed do e.PlaySound(&e.Audio().sounds[1])
	if e.Input().keys['F'] >= .Pressed do e.Audio().sounds[1].pan -= (2 * e.Delta())
	if e.Input().keys['G'] >= .Pressed do e.Audio().sounds[1].pan += (2 * e.Delta())
	if e.Input().keys['S'] == .JustPressed do e.StopSound(&e.Audio().sounds[1])
	if e.Input().keys['A'] == .JustPressed do e.PauseSound(&e.Audio().sounds[1])
	if e.Input().keys['R'] == .JustPressed do e.ResumeSound(&e.Audio().sounds[1])

}

@(export)
GameDraw :: proc() {
	e.ClearScreen({0.4, 0.3, 0.3})

	for i in 0 ..< f32(10.0) {
		e.DrawRectangle({125 + 30 * i, 60 + 30 * i}, {300, 200}, {1 / i, 1 / i, 1 / i, 1})
	}
	e.DrawRectangle(e.GetMouse(), {20, 20}, e.WHITE)
}
