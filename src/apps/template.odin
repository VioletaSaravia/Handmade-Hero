package game

import e "../engine"
import fmt "core:fmt"
import str "core:strings"
import gl "vendor:OpenGL"

s: ^GameState
GameState :: struct {
	imgs:        [2]e.Image,
	mouse:       e.Texture,
	tile_shader: e.Shader,
	tilemap:     Tilemap,
}

Tilemap :: struct {
	tileset:              e.Texture,
	vao, vbo, ebo, i_vbo: u32,
	tilemap:              [TILEMAP_Y][TILEMAP_X]i32,
	instances:            [TILEMAP_X * TILEMAP_Y]TileInstance,
}

TileInstance :: struct {
	pos: [2]f32,
	idx: i32,
}

TILE_SIZE :: 32

@(export)
GameSetup :: proc() {
	e.Settings(
		name = "Test",
		version = "0.1",
		resolution = TILE_SIZE * {TILEMAP_X, TILEMAP_Y}, // 640 x 480
		memory = size_of(GameState),
	)
}

@(export)
GameInit :: proc() {
	s = auto_cast e.GetMemory(0)

	s.mouse = e.NewTexture("pointer.png")
	s.tile_shader, _ = e.NewShader("tiled.vert", "")
	s.tilemap.tileset = e.NewTexture("ship.png")

	quadVertices := [?]f32 {
		0.0,
		0.0,
		0.0,
		0.0, // bottom-left
		1.0,
		0.0,
		1.0,
		0.0, // bottom-right
		1.0,
		1.0,
		1.0,
		1.0, // top-right
		0.0,
		1.0,
		0.0,
		1.0, // top-left
	}
	indices := [?]i32{0, 1, 2, 2, 3, 0}

	gl.GenVertexArrays(1, &s.tilemap.vao)
	gl.GenBuffers(1, &s.tilemap.vbo)
	gl.GenBuffers(1, &s.tilemap.ebo)
	gl.GenBuffers(1, &s.tilemap.i_vbo)
	gl.BindVertexArray(s.tilemap.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, s.tilemap.vbo)
	bla := size_of(f32) * len(quadVertices)
	gl.BufferData(
		gl.ARRAY_BUFFER,
		size_of(f32) * len(quadVertices),
		raw_data(quadVertices[:]),
		gl.STATIC_DRAW,
	)

	gl.VertexAttribPointer(0, 2, gl.FLOAT, gl.FALSE, 4 * size_of(f32), uintptr(0))
	gl.EnableVertexAttribArray(0)
	gl.VertexAttribPointer(1, 2, gl.FLOAT, gl.FALSE, 4 * size_of(f32), uintptr(2 * size_of(f32)))
	gl.EnableVertexAttribArray(1)


	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, s.tilemap.ebo)
	gl.BufferData(
		gl.ELEMENT_ARRAY_BUFFER,
		size_of(i32) * len(indices),
		raw_data(indices[:]),
		gl.STATIC_DRAW,
	)

	for y in 0 ..< TILEMAP_Y do for x in 0 ..< TILEMAP_X {
		i := y * TILEMAP_X + x
		s.tilemap.instances[i].pos = TILE_SIZE_X * {f32(x), f32(y)}
		s.tilemap.instances[i].idx = s.tilemap.tilemap[y][x]
	}

	gl.BindBuffer(gl.ARRAY_BUFFER, s.tilemap.i_vbo)
	gl.BufferData(
		gl.ARRAY_BUFFER,
		size_of(TileInstance) * len(s.tilemap.instances),
		raw_data(s.tilemap.instances[:]),
		gl.DYNAMIC_DRAW,
	)

	gl.VertexAttribPointer(2, 2, gl.FLOAT, gl.FALSE, size_of(TileInstance), uintptr(0))
	gl.EnableVertexAttribArray(2)
	gl.VertexAttribDivisor(2, 1)
	gl.VertexAttribIPointer(3, 1, gl.INT, size_of(TileInstance), uintptr(2 * size_of(f32)))
	gl.EnableVertexAttribArray(3)
	gl.VertexAttribDivisor(3, 1)
}

@(export)
GameUpdate :: proc() {
	when ODIN_DEBUG do s = auto_cast e.GetMemory(0)

	e.ReloadShader(&s.tile_shader)
}

TILEMAP_X :: 20
TILEMAP_Y :: 15
TILE_SIZE_X :: TILE_SIZE
TILE_SIZE_Y :: TILE_SIZE
TILESET_COLS :: 1

@(export)
GameDraw :: proc() {
	when ODIN_DEBUG do s = auto_cast e.GetMemory(0)
	e.ClearScreen({0.4, 0.3, 0.3})

	// tile shader setup
	e.UseShader(s.tile_shader)
	e.SetUniform(s.tile_shader.id, "tileSize", [2]f32{TILE_SIZE_X, TILE_SIZE_Y})
	e.SetUniform(s.tile_shader.id, "tilesetCols", TILESET_COLS)
	e.SetUniform(s.tile_shader.id, "screenSize", e.GetResolution())

	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, s.tilemap.tileset.id)
	gl.BindVertexArray(s.tilemap.vao)

	// on-the-fly tilemap updating
	s.tilemap.instances[0].idx = 3
	gl.BindBuffer(gl.ARRAY_BUFFER, s.tilemap.i_vbo)
	gl.BufferSubData(
		gl.ARRAY_BUFFER,
		0,
		size_of(TileInstance) * len(s.tilemap.instances),
		raw_data(s.tilemap.instances[:]),
	)

	e.SetUniform(s.tile_shader.id, "tex0", 0)
	gl.DrawElementsInstanced(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil, TILEMAP_X * TILEMAP_Y)

	e.UseShader(e.Graphics().shaders[0])
	e.SetUniform(e.Graphics().shaders[0].id, "res", e.GetResolution())
	e.SetUniform(e.Graphics().shaders[0].id, "pos", e.GetMouse())
	e.SetUniform(
		e.Graphics().shaders[0].id,
		"size",
		[2]f32{cast(f32)s.mouse.w, cast(f32)s.mouse.h},
	)
	e.SetUniform(e.Graphics().shaders[0].id, "color", e.WHITE)
	e.UseTexture(s.mouse)
	e.DrawMesh(e.Graphics().square_mesh)
}
