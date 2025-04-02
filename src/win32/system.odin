package engine

import "core:fmt"
import win "core:sys/windows"
import uc "core:unicode/utf16"
import gl "vendor:OpenGL"

WindowBuffer :: struct {
	running:         bool,
	memory:          [^]byte,
	w, h:            i32,
	bytes_per_pixel: i32,
	window:          win.HWND,
	dc:              win.HDC,
	refresh_rate:    u32,
	info:            win.BITMAPINFO,
}

Fullscreen :: proc(window: win.HWND) {
	w := win.GetSystemMetrics(win.SM_CXSCREEN)
	h := win.GetSystemMetrics(win.SM_CYSCREEN)

	style := win.GetWindowLongW(window, win.GWL_STYLE)
	win.SetWindowLongW(window, win.GWL_STYLE, style & ~i32(win.WS_OVERLAPPEDWINDOW))

	win.SetMenu(window, nil)
	win.SetWindowPos(window, win.HWND_TOPMOST, 0, 0, w, h, win.SWP_NOZORDER | win.SWP_FRAMECHANGED)

	ResizeWindow(window, w, h, true)
}

GetRefreshRate :: proc(window: win.HWND) -> u32 {
	monitor := win.MonitorFromWindow(window, .MONITOR_DEFAULTTONEAREST)
	monitor_info: win.MONITORINFOEXW = win.MONITORINFOEXW {
		cbSize = size_of(win.MONITORINFOEXW),
	}

	win.GetMonitorInfoW(monitor, &monitor_info)
	dm := win.DEVMODEW {
		dmSize = size_of(win.DEVMODEW),
	}
	win.EnumDisplaySettingsW(raw_data(monitor_info.szDevice[:]), win.ENUM_CURRENT_SETTINGS, &dm)

	return dm.dmDisplayFrequency
}


ResizeDIBSection :: proc(buffer: ^WindowBuffer, width, height: i32) {
	if buffer.memory != nil do win.VirtualFree(buffer.memory, 0, win.MEM_RELEASE)
	buffer.w = width
	buffer.h = height
	buffer.bytes_per_pixel = 4

	buffer.info = win.BITMAPINFO {
		bmiHeader = {
			biSize          = size_of(buffer.info.bmiHeader),
			biWidth         = buffer.w,
			// Negative height tells Windows to treat the window's y axis as top-down
			biHeight        = -buffer.h,
			biPlanes        = 1,
			biBitCount      = 32, // 4 byte align
			biCompression   = win.BI_RGB,
			biSizeImage     = 0,
			biXPelsPerMeter = 0,
			biYPelsPerMeter = 0,
			biClrUsed       = 0,
			biClrImportant  = 0,
		},
	}

	bitmap_size := buffer.w * buffer.h * buffer.bytes_per_pixel
	buffer.memory = ([^]u8)(
		win.VirtualAlloc(
			nil,
			uint(bitmap_size),
			win.MEM_COMMIT | win.MEM_RESERVE,
			win.PAGE_READWRITE,
		),
	)
}


GetResolution :: proc() -> [2]i32 {
	client_rect: win.RECT
	win.GetClientRect(Mem.Window.window, &client_rect)

	return {client_rect.right - client_rect.left, client_rect.bottom - client_rect.top}
}

ResizeWindow :: proc(hWnd: win.HWND, width, h: i32, fullscreen: bool = false) {
	height := h == 0 ? 1 : h // Prevent division by zero

	rect := win.RECT{0, 0, width, height}

	if !fullscreen {
		win.AdjustWindowRect(
			&rect,
			win.WS_OVERLAPPED | win.WS_CAPTION | win.WS_SYSMENU | win.WS_MINIMIZEBOX,
			win.FALSE,
		)
	}

	win.SetWindowPos(
		hWnd,
		nil,
		0,
		0,
		rect.right - rect.left,
		rect.bottom - rect.top,
		win.SWP_NOMOVE | win.SWP_NOZORDER,
	)

	// win.wglMakeCurrent(hdc, hglrc)
	gl.Viewport(0, 0, width, height)
}


InitWindow :: proc() -> (buffer: WindowBuffer, ok: bool) {
	rect := win.RECT{0, 0, Mem.Settings.resolution.x, Mem.Settings.resolution.y}
	win.AdjustWindowRect(&rect, win.WS_OVERLAPPEDWINDOW, auto_cast false)
	width := rect.right - rect.left
	height := rect.bottom - rect.top
	ResizeDIBSection(&buffer, width, height)
	instance := win.HINSTANCE(win.GetModuleHandleW(nil))

	window_class := win.WNDCLASSW {
		style         = win.CS_HREDRAW | win.CS_VREDRAW | win.CS_OWNDC,
		lpfnWndProc   = MainWindowCallback,
		hInstance     = instance,
		hIcon         = auto_cast win.LoadImageW(
			instance,
			win.L("../../data/icon.ico" when ODIN_DEBUG else "data/icon.ico"),
			win.IMAGE_ICON,
			32,
			32,
			win.LR_LOADFROMFILE,
		),
		lpszClassName = auto_cast raw_data(Mem.Settings.name),
	}
	if win.RegisterClassW(&window_class) == 0 {
		fmt.println("Cannot initialize window")
		return
	}

	buffer.window = win.CreateWindowExW(
		0,
		window_class.lpszClassName,
		auto_cast raw_data(Mem.Settings.name),
		win.WS_OVERLAPPED | win.WS_CAPTION | win.WS_SYSMENU | win.WS_MINIMIZEBOX | win.WS_VISIBLE,
		win.CW_USEDEFAULT,
		win.CW_USEDEFAULT,
		width,
		height,
		nil,
		nil,
		instance,
		nil,
	)
	if buffer.window == nil {
		win.OutputDebugStringA("ERROR")
		return
	}

	buffer.dc = win.GetDC(buffer.window)
	buffer.refresh_rate = GetRefreshRate(buffer.window)
	buffer.running = true
	ok = true

	return
}

GetMouse :: proc() -> [2]f32 {
	pt: win.POINT
	if ok := win.GetCursorPos(&pt); !ok do fmt.println("")

	rect: win.RECT
	if ok := win.GetWindowRect(Mem.Window.window, &rect); !ok do fmt.println("")

	return {f32(pt.x - rect.left - 8), f32(pt.y - rect.top - 31)}
}

LockCursorToWindow :: proc(hWnd: win.HWND) {
	rect: win.RECT
	win.GetClientRect(hWnd, &rect)
	win.ClientToScreen(hWnd, auto_cast &rect.left)
	win.ClientToScreen(hWnd, auto_cast &rect.top)
	win.ClipCursor(&rect)
}
