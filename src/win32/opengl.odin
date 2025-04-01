package engine

import fmt "core:fmt"
import os "core:os"
import fp "core:path/filepath"
import s "core:strings"
import win "core:sys/windows"

import gl "vendor:OpenGL"
import stbi "vendor:stb/image"

SHADER_PATH :: "../../src/shaders/" when ODIN_DEBUG else "shaders/"

InitOpenGL :: proc(window: win.HWND, settings: ^GameSettings) -> (ok: bool = true) {
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

	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
	gl.Enable(gl.BLEND)

	gl.Viewport(0, 0, settings.resolution.x, settings.resolution.y)
	win.ReleaseDC(window, window_dc)
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

SetUniform1i :: proc(id: u32, name: string, value: i32) {
	gl.Uniform1i(gl.GetUniformLocation(id, auto_cast raw_data(name)), value)
}

SetUniform2i :: proc(id: u32, name: string, value: [2]i32) {
	gl.Uniform2i(gl.GetUniformLocation(id, auto_cast raw_data(name)), value.x, value.y)
}

SetUniform1f :: proc(id: u32, name: string, value: f32) {
	gl.Uniform1f(gl.GetUniformLocation(id, auto_cast raw_data(name)), value)
}

SetUniforms1f :: proc(id: u32, name: string, value: []f32) {
	gl.Uniform1fv(
		gl.GetUniformLocation(id, auto_cast raw_data(name)),
		auto_cast len(value),
		raw_data(value),
	)
}

SetUniform2fv :: proc(id: u32, name: string, value: [2]f32) {
	gl.Uniform2f(gl.GetUniformLocation(id, auto_cast raw_data(name)), value.x, value.y)
}

SetUniform3fv :: proc(id: u32, name: string, value: [3]f32) {
	gl.Uniform3f(gl.GetUniformLocation(id, auto_cast raw_data(name)), value.x, value.y, value.z)
}

SetUniform4fv :: proc(id: u32, name: string, value: [4]f32) {
	gl.Uniform4f(
		gl.GetUniformLocation(id, auto_cast raw_data(name)),
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
	SetUniforms1f,
	SetUniform2fv,
	SetUniform3fv,
	SetUniform4fv,
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

	gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGB, new.w, new.h, 0, format, gl.UNSIGNED_BYTE, img)
	gl.GenerateMipmap(gl.TEXTURE_2D)

	stbi.image_free(img)

	return
}

UseTexture :: proc(tex: Texture) {
	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, tex.id)
}

ClearScreen :: proc(color: [3]f32) {
	gl.ClearColor(color.r, color.g, color.b, 1.0)
	gl.Clear(gl.COLOR_BUFFER_BIT)
}

GuiButtonState :: enum {
	Released,
	Pressed,
	Hovered,
}

GuiButton :: proc(pos: [2]u32, size: [2]u32) -> GuiButtonState {
	m := transmute([2]u32)GetMouse()
	hovered := pos.x < m.x && pos.x + size.x > m.x && pos.x < m.y && pos.y + size.y > m.y
	pressed := Input().mouse.left == .Pressed

	if hovered && pressed do return .Pressed
	if hovered do return .Hovered
	return .Released
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

DrawRectangle :: proc(pos: [2]f32, size: [2]f32, color: [4]f32) {
	UseShader(Mem.Graphics.untex_shader)
	SetUniform(Mem.Graphics.untex_shader.id, "color", color)
	SetUniform(Mem.Graphics.untex_shader.id, "pos", pos)
	SetUniform(Mem.Graphics.untex_shader.id, "size", size)
	SetUniform(Mem.Graphics.untex_shader.id, "res", GetResolution())

	gl.BindVertexArray(Mem.Graphics.square_mesh.vao)
	gl.DrawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil)
	gl.BindVertexArray(0)
}

DrawTexture :: proc(tex: Texture, pos: [2]f32, scale: f32 = 1, color: [4]f32 = WHITE) {
	UseTexture(tex)
	UseShader(Mem.Graphics.tex_shader)

	SetUniform(Mem.Graphics.tex_shader.id, "tex0", 0)
	SetUniform(Mem.Graphics.tex_shader.id, "color", color)
	SetUniform(Mem.Graphics.tex_shader.id, "pos", pos)
	SetUniform(Mem.Graphics.tex_shader.id, "size", [2]f32{cast(f32)tex.w, cast(f32)tex.h} * scale)
	SetUniform(Mem.Graphics.tex_shader.id, "res", GetResolution())

	gl.BindVertexArray(Mem.Graphics.square_mesh.vao)
	gl.DrawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil)
	gl.BindVertexArray(0)
}

Tileset :: struct {
	tex:   Texture,
	count: [2]f32,
	size:  [2]f32,
}

Font :: Tileset

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

Anchor :: enum {
	TopLeft,
	TopRight,
	BotLeft,
	BotRight,
	Center,
	CenterLeft,
	CenterRight,
	CenterTop,
	CenterBot,
}

DrawText :: proc(
	font: Font,
	text: string,
	pos: [2]f32 = {0, 0},
	anchor: Anchor = .TopLeft,
	columns: f32 = 1,
	scale: f32 = 1,
	color: [4]f32 = WHITE,
) {

	n_letters: [512]f32
	for l, i in text {
		if i >= 512 do break
		n_letters[i] = BitmapCharmap[l]
	}

	DrawTiles(font, n_letters[:], pos, anchor, {columns, f32(len(text)) / columns}, scale, color)
}

DrawBox :: proc(
	tileset: Tileset,
	pos: [2]f32 = {0, 0},
	size: [2]f32,
	anchor: Anchor = .TopLeft,
	scale: f32 = 1,
) {
	coords := make([]f32, int(size.x * size.y))
	for i in 0 ..< size.x * size.y {
		// TODO: Por qué coords = x es válido???
		switch i {
		case 0:
			coords[i32(i)] = 0
		case size.x * size.y - 1:
			coords[i32(i)] = 8
		case size.x * (size.y - 1):
			coords[i32(i)] = 6
		case size.x * (size.y - 1) + 1 ..< size.x * size.y:
			coords[i32(i)] = 7
		case 1 ..< size.x - 1:
			coords[i32(i)] = 1
		case size.x - 1:
			coords[i32(i)] = 2
		case:
			if (i32(i) % i32(size.x)) == 0 {
				coords[i32(i)] = 3
			} else if (i32(i) % i32(size.x) - 1) == 0 {
				coords[i32(i)] = 5
			} else {
				coords[i32(i)] = 4
			}

		}
	}

	DrawTiles(tileset, coords[:], pos, anchor, size.x, scale)
}

DrawTiles :: proc(
	font: Tileset,
	coords: []f32,
	pos: [2]f32 = {0, 0},
	anchor: Anchor = .TopLeft,
	size: [2]f32,
	scale: f32 = 1,
	color: [4]f32 = WHITE,
) {
	UseTexture(font.tex)
	UseShader(Mem.Graphics.font_shader)
	id := Mem.Graphics.font_shader.id

	SetUniform(id, "coords", coords)

	SetUniform(id, "text_size", size)

	real_size := size * font.size * scale
	SetUniform(id, "size", real_size)

	adjusted_pos: [2]f32
	switch anchor {
	case .TopLeft:
		adjusted_pos = pos
	case .TopRight:
		adjusted_pos = pos - {real_size.x, 0}
	case .BotLeft:
		adjusted_pos = pos - {0, real_size.y}
	case .BotRight:
		adjusted_pos = pos - {real_size.x, real_size.y}
	case .Center:
		adjusted_pos = pos - real_size / 2
	case .CenterLeft:
		adjusted_pos = pos - {0, real_size.y / 2}
	case .CenterRight:
		adjusted_pos = pos - {real_size.x, real_size.y / 2}
	case .CenterTop:
		adjusted_pos = pos - {real_size.x / 2, 0}
	case .CenterBot:
		adjusted_pos = pos - {real_size.x / 2, real_size.y}
	}
	SetUniform(id, "pos", adjusted_pos)

	SetUniform(id, "tex0", 0)
	SetUniform(id, "color", color)
	SetUniform(id, "res", GetResolution())

	SetUniform(id, "font_count", font.count * font.size)
	SetUniform(id, "font_size", font.size)

	gl.BindVertexArray(Mem.Graphics.square_mesh.vao)
	gl.DrawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil)
	gl.BindVertexArray(0)
}

Camera :: [2]f32
