package engine

import win "core:sys/windows"
import uc "core:unicode/utf16"

WindowBuffer :: struct {
	memory:          [^]byte, // Remove?
	w, h:            i32,
	bytes_per_pixel: i32,
	window:          win.HWND,
	dc:              win.HDC,
	refresh_rate:    u32,
	perf_count_freq: i64,
	info:            win.BITMAPINFO,
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


GetWindowDim :: proc(window: win.HWND) -> [2]i32 {
	client_rect: win.RECT
	win.GetClientRect(window, &client_rect)

	return {client_rect.right - client_rect.left, client_rect.bottom - client_rect.top}
}


InitWindow :: proc() -> (buffer: WindowBuffer, ok: bool) {
	ResizeDIBSection(&buffer, Settings.resolution.x, Settings.resolution.y)
	instance := win.HINSTANCE(win.GetModuleHandleW(nil))

	window_class := win.WNDCLASSW {
		style         = win.CS_HREDRAW | win.CS_VREDRAW | win.CS_OWNDC,
		lpfnWndProc   = MainWindowCallback,
		hInstance     = instance,
		hIcon         = nil,
		lpszClassName = win.L("FUCKYOU"),
	}

	if win.RegisterClassW(&window_class) == 0 {
		win.OutputDebugStringA("ERROR")
		return
	}


	buffer.window = win.CreateWindowExW(
		0,
		window_class.lpszClassName,
		win.L("FUCKYOU"),
		win.WS_OVERLAPPEDWINDOW | win.WS_VISIBLE,
		win.CW_USEDEFAULT,
		win.CW_USEDEFAULT,
		Settings.resolution.x,
		Settings.resolution.y,
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
	ok = true
	return
}
