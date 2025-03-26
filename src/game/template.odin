package game

import e "../engine"
import fmt "core:fmt"
import gl "vendor:OpenGL"

State: ^GameState
GameState :: struct {
	player_pos:     e.v2,
	default_shader: e.Shader,
}

@(export)
GameSetup :: proc() {
	e.Settings = {
		name       = "Test",
		version    = "0.1",
		resolution = {800, 600},
		fullscreen = false,
		memory     = 64 * 1000 * 1000,
	}
}

@(export)
GameInit :: proc() {
	State = auto_cast e.Alloc(size_of(GameState))

	// TODO(violeta): Mover al engine
	State.default_shader, _ = e.NewShader("", "")
}

@(export)
GameUpdate :: proc() {}

@(export)
GameDraw :: proc() {
	e.ClearScreen({0.4, 0.3, 0.3})
}
