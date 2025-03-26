package engine

import fmt "core:fmt"
import os "core:os"
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

	gl.Viewport(0, 0, Settings.resolution.x, Settings.resolution.y)
	win.ReleaseDC(window, window_dc)
	return
}

Shader :: struct {
	id: u32,
}

PrintShaderError :: proc(shader: u32) {
	info_log: [^]u8
	len: i32
	gl.GetShaderInfoLog(shader, 512, &len, info_log)
	fmt.println("[SHADER ERROR]", info_log)
}

NewShader :: proc(vert_file: string, frag_file: string) -> (new: Shader, ok: bool) {
	err: i32
	vert_source := os.read_entire_file_from_filename(
		vert_file != "" ? vert_file : SHADER_PATH + "default.vert",
	) or_return
	frag_source := os.read_entire_file_from_filename(
		frag_file != "" ? frag_file : SHADER_PATH + "default.frag",
	) or_return

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

	if gl.GetProgramiv(new.id, gl.LINK_STATUS, &err); err == 0 {
		PrintShaderError(frag_shader)
		return {}, false
	}

	new.id = gl.CreateProgram()
	gl.AttachShader(new.id, vert_shader)
	gl.AttachShader(new.id, frag_shader)
	gl.LinkProgram(new.id)

	gl.DeleteShader(vert_shader)
	gl.DeleteShader(frag_shader)

	return new, true
}

UseShader :: proc(shader: Shader) {
	gl.UseProgram(shader.id)
}

SetUniform :: proc(id: i32, name: string, value: $T) {
	switch t in value {
	case i32:
		gl.Uniform1i(gl.GetUniformLocation(id, name), value)
	case f32:
		gl.Uniform1f(gl.GetUniformLocation(id, name), value)
	case [2]f32:
		gl.Uniform2fv(gl.GetUniformLocation(id, name), value)
	case [3]f32:
		gl.Uniform3fv(gl.GetUniformLocation(id, name), value)
	case _:
		fmt.println("[SHADER ERROR]", "Uniform value type not implemented:", T)
	}
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
	tex: Texture,
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
	pressed := Input.mouse.left == .Pressed

	if hovered && pressed do return .Pressed
	if hovered do return .Hovered
	return .Released
}
