package game

import e "../engine"
import fmt "core:fmt"
import m "core:math"
import gl "vendor:OpenGL"

s: ^GameState
GameState :: struct {
	player_pos:  e.v2,
	default_tex: e.Rectangle,
}

@(export)
GameSetup :: proc() {
	e.Settings({
			name = "Test",
			version = "0.1",
			resolution = {800, 600},
			fullscreen = false,
			memory = size_of(GameState),
		})
}

@(export)
GameInit :: proc() {
	s = auto_cast e.GameGetMemory()
}

@(export)
GameUpdate :: proc() {
	s = auto_cast e.GameGetMemory()
}

@(export)
GameDraw :: proc() {
	e.ClearScreen({0.4, 0.3, 0.3})

	e.DrawRectangle({0, 0}, {300, 300}, {0, 1, 0, 1})
}
