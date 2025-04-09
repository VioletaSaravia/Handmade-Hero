package game

import e "../engine"
import fmt "core:fmt"
import alg "core:math/linalg"
import rd "core:math/rand"
import str "core:strings"
import gl "vendor:OpenGL"

/*
TODO:
- [X] Post-processing shader
- [ ] Move scale to settings
- [X] Optimize tiles VBO
*/

TILES_X :: 80 // 640
TILES_Y :: 45 // 360
TILE :: 8
SCALE :: 2

s: ^GameState
GameState :: struct {
	layer1, layer2: e.Tilemap(TILES_X, TILES_Y),
	ship:           e.Texture,
	pos:            [2]f32,
	textbox:        e.Tilemap(25, 41),
}

@(export)
GameSetup :: proc() {
	e.Settings(
		name = "Test",
		version = "0.1",
		resolution = {TILE * TILES_X * SCALE, TILE * TILES_Y * SCALE},
		memory = size_of(GameState),
	)
}


@(export)
GameInit :: proc() {
	s = auto_cast e.GetMemory(0)

	s.ship = e.NewTexture("ship.png")
	s.textbox = e.NewTilemap("fonts/precise.png", 25, 41, {TILE, TILE})

	tileset := e.NewTexture("micro_tileset.png")
	s.layer1 = e.NewTilemapFromTexture(tileset, TILES_X, TILES_Y, {TILE, TILE})
	s.layer2 = e.NewTilemapFromTexture(tileset, TILES_X, TILES_Y, {TILE, TILE})
	e.TilemapLoadCsv(&s.layer1, "layer1.csv")
	e.TilemapLoadCsv(&s.layer2, "layer2.csv")

	s.pos = {56, 16} * SCALE
}

@(export)
GameUpdate :: proc() {
	when ODIN_DEBUG do s = auto_cast e.GetMemory(0)

	speed := TILE * SCALE * e.Delta() * 4
	if e.Input().keys['W'] >= .Pressed do s.pos.y -= speed
	if e.Input().keys['S'] >= .Pressed do s.pos.y += speed
	if e.Input().keys['A'] >= .Pressed do s.pos.x -= speed
	if e.Input().keys['D'] >= .Pressed do s.pos.x += speed
}

@(export)
GameDraw :: proc() {
	e.DrawTilemap(&s.layer2, scale = SCALE)
	e.DrawTilemap(&s.layer1, scale = SCALE)

	e.DrawTexture(s.ship, s.pos, SCALE)
	e.DrawTexture(s.ship, s.pos + {1, 6} * TILE * SCALE, SCALE)

	text := "With dusk approaching, the fleet decides to advance through the narrow rivers of the region."
	e.DrawText(text, &s.textbox, TILE * {TILES_X - 27, 2}, SCALE)
}
