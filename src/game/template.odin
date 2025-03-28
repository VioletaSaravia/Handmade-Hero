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
	e.Mem.Data = proc() {
		s = auto_cast e.GameGetMemory()
	}
}

@(export)
GameInit :: proc() {
	s.imgs[0], _ = e.LoadImage("door.bmp")
	s.imgs[1], _ = e.LoadImage("ship.bmp")
	e.Mem.Audio.sounds[0] = e.LoadSound("kick.wav")
	e.Mem.Audio.sounds[1] = e.LoadSound("ambience.mp3", .looping)
}

@(export)
GameUpdate :: proc() {
	for k in 'A' ..= 'Z' {
		if e.Mem.Input.keyboard[k] == .JustPressed do fmt.println("Pressed the key", k)
	}

	if e.Mem.Input.keyboard['P'] == .JustReleased do e.PlaySound(&e.Mem.Audio.sounds[0])
	if e.Mem.Input.keyboard['O'] == .JustReleased do e.PlaySound(&e.Mem.Audio.sounds[1])
	if e.Mem.Input.keyboard['F'] == .JustReleased do e.Mem.Audio.sounds[1].pan -= 0.1
	if e.Mem.Input.keyboard['G'] == .JustReleased do e.Mem.Audio.sounds[1].pan += 0.1
	if e.Mem.Input.keyboard['S'] == .JustReleased do e.StopSound(&e.Mem.Audio.sounds[0])
	if e.Mem.Input.keyboard['A'] == .JustReleased do e.PauseSound(&e.Mem.Audio.sounds[0])
	if e.Mem.Input.keyboard['R'] == .JustReleased do e.ResumeSound(&e.Mem.Audio.sounds[0])
}

@(export)
GameDraw :: proc() {
	e.ClearScreen({0.4, 0.3, 0.3})

	for i in 0 ..< f32(10.0) {
		e.DrawRectangle({125 + 30 * i, 60 + 30 * i}, {300, 200}, {1 / i, 1 / i, 1 / i, 1})
	}
}
