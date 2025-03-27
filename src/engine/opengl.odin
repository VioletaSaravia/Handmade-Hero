package engine

import fmt "core:fmt"
import os "core:os"
import s "core:strings"
import win "core:sys/windows"

import gl "vendor:OpenGL"
import stbi "vendor:stb/image"

SHADER_PATH :: "../../shaders/" when ODIN_DEBUG else "shaders/"

InitOpenGL :: proc(window: win.HWND) -> (ok: bool = true) {
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

	gl.Viewport(0, 0, Mem.Settings.resolution.x, Mem.Settings.resolution.y)
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
	fmt.println("[SHADER ERROR]", info_log[:])
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
SetUniform1f :: proc(id: u32, name: string, value: f32) {
	gl.Uniform1f(gl.GetUniformLocation(id, auto_cast raw_data(name)), value)
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
	SetUniform1f,
	SetUniform2fv,
	SetUniform3fv,
	SetUniform4fv,
}

Texture :: struct {
	id:           u32,
	w, h, n_chan: i32,
}

NewTexture :: proc(filepath: string) -> (new: Texture, ok: bool) {
	data: []byte
	data, ok = os.read_entire_file_from_filename(filepath)
	if !ok {
		return {}, ok
	}
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
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)

	gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGB, new.w, new.h, 0, gl.RGB, gl.UNSIGNED_BYTE, img)
	gl.GenerateMipmap(gl.TEXTURE_2D)

	stbi.image_free(img)

	return
}

UseTexture :: proc(tex: Texture) {
	gl.ActiveTexture(gl.TEXTURE0)
	gl.BindTexture(gl.TEXTURE_2D, tex.id)
}

Object :: struct {
	vao: u32,
	tex: Rectangle,
}

NewObject :: proc() -> Object {
	return Object{}
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
	pressed := Mem.Input.mouse.left == .Pressed

	if hovered && pressed do return .Pressed
	if hovered do return .Hovered
	return .Released
}

Rectangle :: struct {
	ebo, vbo, vao: u32,
}

@(rodata)
vertices := [?]f32{1, 1, 0, 1, -1, 0, -1, -1, 0, -1, 1, 0}
@(rodata)
indices := [?]u32{0, 1, 3, 1, 2, 3}

NewRectangle :: proc() -> (new: Rectangle) {
	gl.GenBuffers(1, &new.ebo)
	gl.GenBuffers(1, &new.vbo)
	gl.GenVertexArrays(1, &new.vao)

	gl.BindVertexArray(new.vao)

	gl.BindBuffer(gl.ARRAY_BUFFER, new.vbo)
	gl.BufferData(gl.ARRAY_BUFFER, size_of(vertices), raw_data(vertices[:]), gl.STATIC_DRAW)
	gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, new.ebo)
	gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, size_of(indices), raw_data(indices[:]), gl.STATIC_DRAW)

	gl.VertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 3 * size_of(f32), uintptr{})
	gl.EnableVertexAttribArray(0)

	return
}


RectToDraw :: struct {
	pos, size: [2]f32,
	color:     [4]f32,
}
RectDrawQueue: [dynamic]RectToDraw

DrawRectangle :: proc(pos: [2]f32, size: [2]f32, color: [4]f32) {
	UseShader(Mem.Shaders[0])
	SetUniform(Mem.Shaders[0].id, "color", color)
	SetUniform(Mem.Shaders[0].id, "pos", pos)
	SetUniform(Mem.Shaders[0].id, "size", size)

	gl.BindVertexArray(Mem.DefaultRect.vao)
	gl.DrawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, nil)
	gl.BindVertexArray(0)

	for rect in RectDrawQueue {
		// TODO(violeta): Arena draw queue
	}
}
