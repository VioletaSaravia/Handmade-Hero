package engine

import fmt "core:fmt"
import os "core:os"
import fp "core:path/filepath"
import s "core:strings"
import win "core:sys/windows"

import gl "vendor:OpenGL"
import stbi "vendor:stb/image"

SHADER_PATH :: "../../src/shaders/" when ODIN_DEBUG else "shaders/"

InitOpenGL :: proc(window: win.HWND, settings: ^GameSettings) -> bool {
	desired_pixel_format := win.PIXELFORMATDESCRIPTOR {
		nSize      = size_of(win.PIXELFORMATDESCRIPTOR),
		nVersion   = 1,
		dwFlags    = win.PFD_SUPPORT_OPENGL | win.PFD_DRAW_TO_WINDOW | win.PFD_DOUBLEBUFFER,
		iPixelType = win.PFD_TYPE_RGBA,
		cColorBits = 32,
		cAlphaBits = 8,
	}

	window_dc := win.GetDC(window)
	suggested_pixel_format_index := win.ChoosePixelFormat(window_dc, &desired_pixel_format)
	suggested_pixel_format: win.PIXELFORMATDESCRIPTOR
	win.SetPixelFormat(window_dc, suggested_pixel_format_index, &suggested_pixel_format)

	opengl_rc := win.wglCreateContext(window_dc)
	win.wglMakeCurrent(window_dc, opengl_rc) or_return
	gl.load_up_to(4, 6, win.gl_set_proc_address)

	gl.Enable(gl.BLEND)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

	gl.Viewport(0, 0, settings.resolution.x, settings.resolution.y)
	win.ReleaseDC(window, window_dc)
	return true
}

InitGraphics :: proc(
	window: win.HWND,
	settings: ^GameSettings,
) -> (
	new: GraphicsBuffer,
	ok: bool = true,
) {
	InitOpenGL(window, settings) or_return

	new.shaders[.Default] = NewShader("", "") or_return
	new.shaders[.Tiled] = NewShader("tiled.vert", "") or_return
	new.square_mesh = NewMesh(square_vertices[:], square_indices[:])
	new.mouse = NewTexture("pointer.png")

	return
}

when ODIN_DEBUG {
	Shader :: struct {
		id:         u32,
		vert_path:  string,
		vert_write: u64,
		frag_path:  string,
		frag_write: u64,
	}
} else {
	Shader :: struct {
		id: u32,
	}
}

PrintShaderError :: proc(shader: u32) {
	info_log := make([^]u8, 512)
	len: i32
	gl.GetShaderInfoLog(shader, 512, &len, info_log)
	fmt.println("[SHADER ERROR]", info_log[:len])
}

NewShader :: proc(vert_file: string, frag_file: string) -> (new: Shader, ok: bool) {
	err: i32
	vert_filename := (vert_file != "" ? vert_file : "default.vert")
	frag_filename := (frag_file != "" ? frag_file : "default.frag")
	vert_path, _ := s.concatenate({SHADER_PATH, vert_filename})
	frag_path, _ := s.concatenate({SHADER_PATH, frag_filename})

	when ODIN_DEBUG {
		new.vert_path = vert_filename
		new.frag_path = frag_filename
		v_write, _ := os.last_write_time_by_name(vert_path)
		f_write, _ := os.last_write_time_by_name(frag_path)
		new.vert_write = u64(v_write)
		new.frag_write = u64(f_write)
	}

	vert_source: []byte
	frag_source: []byte
	for {
		// TODO(violeta): Por qué hay que hacer esto?
		vert_source = os.read_entire_file_from_filename(vert_path) or_return
		frag_source = os.read_entire_file_from_filename(frag_path) or_return
		if vert_source != nil && frag_source != nil do break
	}

	vert_shader := gl.CreateShader(gl.VERTEX_SHADER)
	gl.ShaderSource(vert_shader, 1, auto_cast &vert_source, nil)
	gl.CompileShader(vert_shader)
	if gl.GetShaderiv(vert_shader, gl.COMPILE_STATUS, &err); err == 0 {
		PrintShaderError(vert_shader)
		return {}, false
	}

	frag_shader := gl.CreateShader(gl.FRAGMENT_SHADER)
	gl.ShaderSource(frag_shader, 1, auto_cast &frag_source, nil)
	gl.CompileShader(frag_shader)
	if gl.GetShaderiv(frag_shader, gl.COMPILE_STATUS, &err); err == 0 {
		PrintShaderError(frag_shader)
		return {}, false
	}

	new.id = gl.CreateProgram()
	gl.AttachShader(new.id, vert_shader)
	gl.AttachShader(new.id, frag_shader)
	gl.LinkProgram(new.id)
	if gl.GetProgramiv(new.id, gl.LINK_STATUS, &err); err == 0 {
		return {}, false
	}

	gl.DeleteShader(vert_shader)
	gl.DeleteShader(frag_shader)

	return new, true
}

UseShader :: proc(shader: Shader) {
	gl.UseProgram(shader.id)
	Graphics().active_shader = shader.id
}

ReloadShader :: proc(shader: ^Shader) -> os.Error {
	when ODIN_DEBUG {
		vert_path := s.concatenate({SHADER_PATH, shader.vert_path}) or_return
		frag_path := s.concatenate({SHADER_PATH, shader.frag_path}) or_return
		vert_time: u64 = auto_cast os.last_write_time_by_name(vert_path) or_return
		frag_time: u64 = auto_cast os.last_write_time_by_name(frag_path) or_return

		if vert_time <= shader.vert_write && frag_time <= shader.frag_write do return nil
		if new_shader, ok := NewShader(shader.vert_path, shader.frag_path); !ok {
			shader.vert_write = new_shader.vert_write
			shader.frag_write = new_shader.frag_write
			// TODO(violeta): Agregar errores de OpenGL?
			return os.General_Error.Invalid_File
		} else {
			shader^ = new_shader
		}
	}

	return nil
}

SetUniform1i :: proc(name: string, value: i32) {
	gl.Uniform1i(gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)), value)
}

SetUniform2i :: proc(name: string, value: [2]i32) {
	gl.Uniform2i(
		gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)),
		value.x,
		value.y,
	)
}

SetUniform1f :: proc(name: string, value: f32) {
	gl.Uniform1f(gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)), value)
}

SetUniform1fv :: proc(name: string, value: []f32) {
	gl.Uniform1fv(
		gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)),
		auto_cast len(value),
		raw_data(value),
	)
}

SetUniform2f :: proc(name: string, value: [2]f32) {
	gl.Uniform2f(
		gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)),
		value.x,
		value.y,
	)
}

SetUniform3f :: proc(name: string, value: [3]f32) {
	gl.Uniform3f(
		gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)),
		value.x,
		value.y,
		value.z,
	)
}

SetUniform4f :: proc(name: string, value: [4]f32) {
	gl.Uniform4f(
		gl.GetUniformLocation(Graphics().active_shader, auto_cast raw_data(name)),
		value.x,
		value.y,
		value.z,
		value.w,
	)
}

SetUniform :: proc {
	SetUniform1i,
	SetUniform2i,
	SetUniform1f,
	SetUniform1fv,
	SetUniform2f,
	SetUniform3f,
	SetUniform4f,
}

Texture :: struct {
	id:           u32,
	w, h, n_chan: i32,
}

NewTexture :: proc($path: string) -> (new: Texture) {
	data := #load(DATA + path, []byte)

	img := stbi.load_from_memory(
		raw_data(data),
		auto_cast len(data),
		&new.w,
		&new.h,
		&new.n_chan,
		0,
	)

	gl.GenTextures(1, &new.id)
	gl.BindTexture(gl.TEXTURE_2D, new.id)

	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	format: u32 = fp.ext(path) == ".bmp" ? gl.RGB : gl.RGBA

	gl.TexImage2D(gl.TEXTURE_2D, 0, i32(format), new.w, new.h, 0, format, gl.UNSIGNED_BYTE, img)
	// gl.GenerateMipmap(gl.TEXTURE_2D)

	stbi.image_free(img)

	return
}

UseTexture :: proc(tex: Texture) {
	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, tex.id)
}

DrawTexture :: proc(tex: Texture, pos: [2]f32 = {}, scale: f32 = 1) {
	UseShader(Graphics().shaders[.Default])
	SetUniform("res", GetResolution())
	SetUniform("pos", pos)
	SetUniform("scale", scale)
	SetUniform("size", [2]f32{f32(tex.w), f32(tex.h)})
	SetUniform("color", WHITE)
	UseTexture(tex)
	DrawMesh(Graphics().square_mesh)
}

ClearScreen :: proc(color: [4]f32 = 0) {
	gl.ClearColor(color.r, color.g, color.b, color.a)
	gl.Clear(gl.COLOR_BUFFER_BIT)
}

Mesh :: struct {
	// TODO: Hace falta almacenar el EBO y el VBO?
	ebo, vbo, vao: u32,
}

@(rodata)
square_vertices := [?]f32{1, 1, 0, 1, 0, 1, -1, 0, 1, 1, -1, -1, 1, 0, 1, -1, 1, 0, 0, 0}
@(rodata)
square_indices := [?]u32{0, 1, 3, 1, 2, 3}

NewMesh :: proc(vertices: []f32, indices: []u32) -> (new: Mesh) {
	gl.GenBuffers(1, &new.ebo)
	gl.GenBuffers(1, &new.vbo)
	gl.GenVertexArrays(1, &new.vao)

	gl.BindVertexArray(new.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, new.vbo)
	gl.BufferData(
		gl.ARRAY_BUFFER,
		size_of(f32) * len(vertices),
		raw_data(vertices),
		gl.STATIC_DRAW,
	)
	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, new.ebo)
	gl.BufferData(
		gl.ELEMENT_ARRAY_BUFFER,
		size_of(u32) * len(indices),
		raw_data(indices),
		gl.STATIC_DRAW,
	)

	stride: i32 = 5 * size_of(f32)
	gl.VertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, stride, 0)
	gl.EnableVertexAttribArray(0)

	gl.VertexAttribPointer(1, 2, gl.FLOAT, gl.FALSE, stride, (3 * size_of(f32)))
	gl.EnableVertexAttribArray(1)

	return
}

DrawMesh :: proc(mesh: Mesh) {
	gl.BindVertexArray(mesh.vao)
	gl.DrawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil)
	gl.BindVertexArray(0)
}

Tilemap :: struct($x, $y: u32) {
	tileset:              Texture,
	tile_size:            [2]f32,
	vao, vbo, ebo, i_vbo: u32,
	instances:            [x * y]TileInstance,
}

TileInstance :: struct {
	pos: [2]f32,
	idx: i32,
}


@(rodata)
quad_vertices := [?]f32{0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1}
@(rodata)
quad_indices := [?]i32{0, 1, 2, 2, 3, 0}

NewTilemap :: proc($tileset_path: string, $x, $y: u32, tile_size: [2]f32) -> (new: Tilemap(x, y)) {
	new.tileset = NewTexture(tileset_path)
	new.tile_size = tile_size

	gl.GenVertexArrays(1, &new.vao)
	gl.GenBuffers(1, &new.vbo)
	gl.GenBuffers(1, &new.ebo)
	gl.GenBuffers(1, &new.i_vbo)
	gl.BindVertexArray(new.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, new.vbo)
	gl.BufferData(
		gl.ARRAY_BUFFER,
		size_of(f32) * len(quad_vertices),
		raw_data(quad_vertices[:]),
		gl.STATIC_DRAW,
	)

	gl.VertexAttribPointer(0, 2, gl.FLOAT, gl.FALSE, 4 * size_of(f32), uintptr(0))
	gl.EnableVertexAttribArray(0)
	gl.VertexAttribPointer(1, 2, gl.FLOAT, gl.FALSE, 4 * size_of(f32), uintptr(2 * size_of(f32)))
	gl.EnableVertexAttribArray(1)

	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, new.ebo)
	gl.BufferData(
		gl.ELEMENT_ARRAY_BUFFER,
		size_of(i32) * len(quad_indices),
		raw_data(quad_indices[:]),
		gl.STATIC_DRAW,
	)

	for iy in 0 ..< y do for ix in 0 ..< x {
		i := iy * x + ix
		new.instances[i].pos = [2]f32{new.tile_size.x * f32(ix), new.tile_size.y * f32(iy)}
	}

	gl.BindBuffer(gl.ARRAY_BUFFER, new.i_vbo)
	gl.BufferData(
		gl.ARRAY_BUFFER,
		size_of(TileInstance) * len(new.instances),
		raw_data(new.instances[:]),
		gl.DYNAMIC_DRAW,
	)

	gl.VertexAttribPointer(2, 2, gl.FLOAT, gl.FALSE, size_of(TileInstance), uintptr(0))
	gl.EnableVertexAttribArray(2)
	gl.VertexAttribDivisor(2, 1)

	gl.VertexAttribIPointer(3, 1, gl.INT, size_of(TileInstance), uintptr(2 * size_of(f32)))
	gl.EnableVertexAttribArray(3)
	gl.VertexAttribDivisor(3, 1)

	return
}

DrawTilemap :: proc(tilemap: ^Tilemap($x, $y), pos: [2]f32 = {}, scale: f32 = 2) {
	UseShader(Graphics().shaders[.Tiled])
	SetUniform("tile_size", tilemap.tile_size)
	tileset_size := [2]i32 {
		tilemap.tileset.w / i32(tilemap.tile_size.x),
		tilemap.tileset.h / i32(tilemap.tile_size.y),
	}
	SetUniform("tileset_size", tileset_size)
	SetUniform("res", GetResolution())
	SetUniform("pos", pos)
	SetUniform("scale", scale)
	SetUniform("tex0", 0)

	UseTexture(tilemap.tileset)

	gl.BindVertexArray(tilemap.vao)

	// on-the-fly tilemap updating
	gl.BindBuffer(gl.ARRAY_BUFFER, tilemap.i_vbo)
	gl.BufferSubData(
		gl.ARRAY_BUFFER,
		0,
		size_of(TileInstance) * len(tilemap.instances),
		raw_data(tilemap.instances[:]),
	)

	gl.DrawElementsInstanced(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil, len(tilemap.instances))
}

BitmapCharmap := [256]f32 {
	' '  = 0,
	'A'  = 1,
	'B'  = 2,
	'C'  = 3,
	'D'  = 4,
	'E'  = 5,
	'F'  = 6,
	'G'  = 7,
	'H'  = 8,
	'I'  = 9,
	'J'  = 10,
	'K'  = 11,
	'L'  = 12,
	'M'  = 13,
	'N'  = 14,
	'O'  = 15,
	'P'  = 16,
	'Q'  = 17,
	'R'  = 18,
	'S'  = 19,
	'T'  = 20,
	'U'  = 21,
	'V'  = 22,
	'W'  = 23,
	'X'  = 24,
	'Y'  = 25,
	'Z'  = 26,
	'?'  = 27,
	'!'  = 28,
	'.'  = 29,
	','  = 30,
	'-'  = 31,
	'+'  = 32,
	'a'  = 32 + 1,
	'b'  = 32 + 2,
	'c'  = 32 + 3,
	'd'  = 32 + 4,
	'e'  = 32 + 5,
	'f'  = 32 + 6,
	'g'  = 32 + 7,
	'h'  = 32 + 8,
	'i'  = 32 + 9,
	'j'  = 32 + 10,
	'k'  = 32 + 11,
	'l'  = 32 + 12,
	'm'  = 32 + 13,
	'n'  = 32 + 14,
	'o'  = 32 + 15,
	'p'  = 32 + 16,
	'q'  = 32 + 17,
	'r'  = 32 + 18,
	's'  = 32 + 19,
	't'  = 32 + 20,
	'u'  = 32 + 21,
	'v'  = 32 + 22,
	'w'  = 32 + 23,
	'x'  = 32 + 24,
	'y'  = 32 + 25,
	'z'  = 32 + 26,
	':'  = 32 + 27,
	';'  = 32 + 28,
	'"'  = 32 + 29,
	'('  = 32 + 30,
	')'  = 32 + 31,
	'@'  = 64,
	'&'  = 64 + 1,
	'1'  = 64 + 2,
	'2'  = 64 + 3,
	'3'  = 64 + 4,
	'4'  = 64 + 5,
	'5'  = 64 + 6,
	'6'  = 64 + 7,
	'7'  = 64 + 8,
	'8'  = 64 + 9,
	'9'  = 64 + 10,
	'0'  = 64 + 11,
	'%'  = 64 + 12,
	'^'  = 64 + 13,
	'*'  = 64 + 14,
	'{'  = 64 + 15,
	'}'  = 64 + 16,
	'='  = 64 + 17,
	'#'  = 64 + 18,
	'/'  = 64 + 19,
	'\\' = 64 + 20,
	'$'  = 64 + 21,
	'£'  = 64 + 22,
	'['  = 64 + 23,
	']'  = 64 + 24,
	'<'  = 64 + 25,
	'>'  = 64 + 26,
	'\'' = 64 + 27,
	'`'  = 64 + 28,
	'~'  = 64 + 29,
}

DrawText :: proc(text: string, tilemap: ^Tilemap($x, $y), pos: [2]f32 = {}, scale: f32 = 2) {
	text_i, box_i, next_space: int
	add_whitespace: bool
	for text_i < len(text) && box_i < int(x * y) {
		if !add_whitespace do for i in text_i ..< len(text) {
			if text[i] == ' ' || (i == len(text) - 1) {
				next_space = i - text_i
				break
			}
		}

		add_whitespace = u32(next_space) >= x - (auto_cast box_i % x)

		tilemap.instances[box_i].idx =
			auto_cast BitmapCharmap[text[text_i]] if !add_whitespace else 0

		text_i += !add_whitespace ? 1 : 0
		box_i += 1

		add_whitespace = add_whitespace && ((box_i % auto_cast x) != 0)
	}

	if len(text) < int(x * y) {
		for i in box_i ..< int(x * y) do tilemap.instances[i].idx = 0
	}

	DrawTilemap(tilemap, pos, scale)
}


WHITE :: [4]f32{1, 1, 1, 1}

BMPColor :: struct #packed {
	b, g, r, a: u8,
}

BMPCompression :: enum u32 {
	RGB,
	RLE8,
	RLE4,
	BITFIELDS,
	JPEG,
	PNG,
	ALPHABITFIELDS,
	CMYK,
	CMYKRLE8,
	CMYKRLE4,
}

AseBMP32x32 :: struct #packed {
	// BMFH
	bf_type:        u16,
	bf_size:        u32,
	bf_reserved1:   u16,
	bf_reserved2:   u16,
	bf_off_bits:    u32,

	// BMIH
	size:           u32,
	width:          i32,
	height:         i32,
	planes:         u16,
	bit_count:      u16,
	compression:    BMPCompression,
	size_image:     u32,
	pels_per_meter: [2]i32,
	clr_used:       u32,
	clr_important:  u32,

	// DATA
	rgbq:           [256]BMPColor,
	line_data:      [1024]byte,
}

Image :: []byte

LoadImage :: proc($path: string) -> (result: Image, ok: bool) {
	data := raw_data(#load(DATA + path, []byte))

	switch fp.ext(path) {
	case ".bmp":
		data := transmute(^AseBMP32x32)data
		result = make([]u8, 1024 * 4)
		for color, i in data.rgbq {
			id := i * 4
			result[id + 0] = color.r
			result[id + 1] = color.g
			result[id + 2] = color.b
			result[id + 3] = color.a
		}
		return result, true
	case:
	}

	return result, false
}
