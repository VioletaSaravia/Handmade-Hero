package a_game

import e "../../src/engine"
import fmt "core:fmt"
import gl "vendor:OpenGL"

@(export)
GameSetup :: proc() {
}

@(export)
GameInit :: proc() {}

@(export)
GameUpdate :: proc() {}

@(export)
GameDraw :: proc() {
	e.ClearScreen(e.WHITE.xyz)
}
