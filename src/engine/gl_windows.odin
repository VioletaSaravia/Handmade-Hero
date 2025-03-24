package engine

import win "core:sys/windows"
import gl "vendor:OpenGL"

InitOpenGL :: proc(window: win.HWND) -> (ok: bool) {
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
	// gl.Viewport(0, 0, Settings.resolution.x, Settings.resolution.y)
	win.ReleaseDC(window, window_dc)
	ok = true
	return
}

Draw :: proc() {
	// gl.ClearColor(0.2, 0.3, 0.3, 1.0)
	// gl.Clear(gl.COLOR_BUFFER_BIT)
}


Shader :: struct {
	id: u32,
}

NewShader :: proc(vert_file: string, frag_file: string) -> Shader {
	new: Shader
	ok: i32
	vert_source: cstring
	frag_source: cstring

	vert_shader := gl.CreateShader(gl.VERTEX_SHADER)
	gl.ShaderSource(vert_shader, 1, &vert_source, nil)
	gl.CompileShader(vert_shader)

	if gl.GetShaderiv(vert_shader, gl.COMPILE_STATUS, &ok); ok == 0 {
		/* TODO(violeta): log */
	}

	frag_shader := gl.CreateShader(gl.FRAGMENT_SHADER)
	gl.ShaderSource(frag_shader, 1, &frag_source, nil)
	gl.CompileShader(frag_shader)

	new.id = gl.CreateProgram()
	gl.AttachShader(new.id, vert_shader)
	gl.AttachShader(new.id, frag_shader)
	gl.LinkProgram(new.id)

	if gl.GetProgramiv(new.id, gl.LINK_STATUS, &ok); ok == 0 {
		/* TODO(violeta): log */
	}

	gl.DeleteShader(vert_shader)
	gl.DeleteShader(frag_shader)
	// Free memory

	return new
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
	}
}

Texture :: struct {
	id:           u32,
	w, h, n_chan: i32,
}

Object :: struct {
	vao: u32,
	tex: Texture,
}

NewObject :: proc() -> Object {
	return Object{}
}