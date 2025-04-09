package game

import e "../engine"
import fmt "core:fmt"
import alg "core:math/linalg"
import rd "core:math/rand"
import gl "vendor:OpenGL"

s: ^GameState
GameState :: struct {
	ship: e.Texture,
	pos:  [2]f32,
}

@(export)
GameSetup :: proc() {
	e.Settings(
		name = "Test",
		version = "0.1",
		resolution = {640, 480},
		memory = size_of(GameState),
	)
}

@(export)
GameInit :: proc() {
	s = auto_cast e.GetMemory(0)

	s.ship = e.NewTexture("ship.png")
	s.pos = {48, 0}
}

@(export)
GameUpdate :: proc() {
	when ODIN_DEBUG do s = auto_cast e.GetMemory(0)

	speed := 32 * e.Delta() * 4
	if e.Input().keys['W'] >= .Pressed do s.pos.y -= speed
	if e.Input().keys['S'] >= .Pressed do s.pos.y += speed
	if e.Input().keys['A'] >= .Pressed do s.pos.x -= speed
	if e.Input().keys['D'] >= .Pressed do s.pos.x += speed
}

@(export)
GameDraw :: proc() {
	e.DrawTexture(s.ship, s.pos)
}
