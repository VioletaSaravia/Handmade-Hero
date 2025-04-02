package game

import e "../win32"
import fmt "core:fmt"
import str "core:strings"
import gl "vendor:OpenGL"

s: ^GameState
GameState :: struct {
	player_pos: [2]f32,
	imgs:       [2]e.Image,
	mouse:      e.Texture,
	ship:       e.Texture,
	box:        e.Tileset,
	font:       e.Font,
}

TILE_SIZE :: 24

@(export)
GameSetup :: proc() {
	e.Settings(
		{
			name = "Test",
			version = "0.1",
			resolution = {TILE_SIZE * 34, TILE_SIZE * 25},
			fullscreen = false,
			memory = size_of(GameState) * 5,
		},
	)

	e.Mem.LoadData = proc() {
		s = auto_cast e.GetMemory(0)
	}
}

@(export)
GameInit :: proc() {
	s.imgs[0], _ = e.LoadImage("door.bmp")
	s.mouse = e.NewTexture("pointer.png")
	s.ship = e.NewTexture("ship.png")

	s.box = e.Tileset{e.NewTexture("box.bmp"), {3, 3}, {24, 24}}
	s.font = e.Font{e.NewTexture("fonts/precise_3x.png"), {32, 3}, {8, 8}}

	e.Audio().sounds[0] = e.LoadSound("kick.wav")
	e.Audio().sounds[1] = e.LoadSound("ambience.mp3", .looping)
}

@(export)
GameUpdate :: proc() {
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
		e.DrawRectangle({125 + 30 * i, 80 + 30 * i}, {300, 200}, {1 / i, 1 / i, 1 / i, 1})
	}

	set: [10]f32 = 0
	e.DrawTiles(e.Tileset{tex = s.ship, count = {1, 1}, size = {32, 32}}, set[:], size = {10, 10})

	e.DrawText(
		s.font,
		"Violeta Engine v0.1",
		pos = {auto_cast e.GetResolution().x / 2, 24},
		anchor = .CenterTop,
		scale = 2,
		columns = 19,
	)
	e.DrawTexture(s.mouse, e.GetMouse())
}
