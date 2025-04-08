package game

import e "../engine"
import csv "core:encoding/csv"
import fmt "core:fmt"
import alg "core:math/linalg"
import rd "core:math/rand"
import "core:strconv"
import str "core:strings"
import gl "vendor:OpenGL"

TILES_X :: 30
TILES_Y :: 20
TILE :: 8
SCALE :: 2

s: ^GameState
GameState :: struct {
	imgs:           [2]e.Image,
	layer1, layer2: e.Tilemap(TILES_X, TILES_Y),
	ship:           e.Texture,
	pos:            [2]f32,
	textbox:        e.Tilemap(TILES_X - 2, 5),
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
	s.textbox = e.NewTilemap("fonts/precise.png", TILES_X - 2, 5, {TILE, TILE})
	s.layer1 = e.NewTilemap("micro_tileset.png", TILES_X, TILES_Y, {TILE, TILE})
	s.layer2 = e.NewTilemap("micro_tileset.png", TILES_X, TILES_Y, {TILE, TILE})

	s.pos = {56, 16} * SCALE

	{
		r: csv.Reader
		r.reuse_record = true
		r.reuse_record_buffer = true
		defer csv.reader_destroy(&r)

		layer1_data := #load(e.DATA + "layer1.csv")
		csv.reader_init_with_string(&r, string(layer1_data))

		for r, i, err in csv.iterator_next(&r) {
			if err != nil do break
			for f, j in r {
				fmt.printfln("Record %v, field %v: %q", i, j, f)
				num := strconv.atoi(f)
				s.layer1.instances[i * TILES_X + j].idx = auto_cast num if num != -1 else 0
			}
		}
	}

	{
		r: csv.Reader
		r.reuse_record = true
		r.reuse_record_buffer = true
		defer csv.reader_destroy(&r)

		layer2_data := #load(e.DATA + "layer2.csv")
		csv.reader_init_with_string(&r, string(layer2_data))

		for r, i, err in csv.iterator_next(&r) {
			if err != nil do break
			for f, j in r {
				fmt.printfln("Record %v, field %v: %q", i, j, f)
				num := strconv.atoi(f)
				s.layer2.instances[i * TILES_X + j].idx = auto_cast num if num != -1 else 0
			}
		}
	}
}

@(export)
GameUpdate :: proc() {
	when ODIN_DEBUG do s = auto_cast e.GetMemory(0)

	if e.Input().keys['W'] == .JustPressed do s.pos.y -= TILE * SCALE
	if e.Input().keys['S'] == .JustPressed do s.pos.y += TILE * SCALE
	if e.Input().keys['A'] == .JustPressed do s.pos.x -= TILE * SCALE
	if e.Input().keys['D'] == .JustPressed do s.pos.x += TILE * SCALE
}

@(export)
GameDraw :: proc() {
	// when ODIN_DEBUG do s = auto_cast e.GetMemory(0)

	e.DrawTilemap(&s.layer2, scale = 4)
	e.DrawTilemap(&s.layer1, scale = 4)
	e.DrawTexture(s.ship, s.pos, SCALE)
	e.DrawTexture(s.ship, s.pos + {1, 6} * TILE * SCALE, SCALE)
	text := "With dusk approaching, the fleet decides to advance through the narrow rivers of the region."
	e.DrawText(text, &s.textbox, {TILE, TILE * (TILES_Y - 1 - 5)}, 4)
}
