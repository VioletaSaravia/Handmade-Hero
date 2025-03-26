package a_game

import e "../../src/engine"
import fmt "core:fmt"
import gl "vendor:OpenGL"

@(export)
GameSetup :: proc() {
	e.Settings = {
		name       = "Test",
		version    = "0.1",
		resolution = {800, 600},
		memory     = 64 * 1000 * 1000,
	}
}

@(export)
GameInit :: proc() {}

@(export)
GameUpdate :: proc() {}

@(export)
GameDraw :: proc() {
	e.ClearScreen({0.4, 0.3, 0.3})
}
