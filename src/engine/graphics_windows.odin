package engine

import win "core:sys/windows"

import gl "vendor:OpenGL"

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
	new.shaders[.Tiled] = NewShader("tiled.vert", "tiled.frag") or_return
	new.square_mesh = NewMesh(square_vertices[:], square_indices[:])
	new.mouse = NewTexture("pointer.png")
	new.post_shader = NewPostShader("post.frag")

	return
}
