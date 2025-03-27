package engine

import "core:fmt"
import win "core:sys/windows"
import uc "core:unicode/utf16"

ReadEntireFile :: proc(filename: ^string) -> (result: rawptr, ok: bool) {
	handle := win.CreateFileW(
		auto_cast filename,
		win.GENERIC_READ,
		0,
		nil,
		win.OPEN_EXISTING,
		win.FILE_ATTRIBUTE_READONLY,
		win.HANDLE{},
	)

	if handle == win.INVALID_HANDLE_VALUE {
		return nil, false
	}

	size: win.LARGE_INTEGER
	size_read: win.LARGE_INTEGER

	if ok = auto_cast win.GetFileSizeEx(handle, &size); !ok || size == 0 {
		return nil, false
	}

	result = win.VirtualAlloc(
		nil,
		auto_cast size,
		win.MEM_COMMIT | win.MEM_RESERVE,
		win.PAGE_READWRITE,
	)

	// TODO(violeta): Este auto_cast está bien?
	if ok = auto_cast win.ReadFile(handle, result, auto_cast size, auto_cast &size_read, nil);
	   !ok {
		return nil, ok
	}

	win.CloseHandle(handle)

	return result, size == size_read
}

// TODO(violeta): No testeado
WriteEntireFile :: proc(filename: ^string, size: u32, memory: rawptr) -> (ok: bool) {
	handle := win.CreateFileW(
		auto_cast filename,
		win.GENERIC_WRITE,
		0,
		nil,
		win.CREATE_ALWAYS,
		win.FILE_ATTRIBUTE_NORMAL,
		nil,
	)
	if handle == win.INVALID_HANDLE_VALUE {
		// LOG
		return false
	}

	size_written: win.DWORD
	ok = auto_cast win.WriteFile(handle, memory, size, &size_written, nil)

	win.CloseHandle(handle)

	if !ok || size_written != size {
		// LOG
		return false
	}
	return
}

FreeFileMemory :: proc(memory: rawptr) -> bool {
	return auto_cast win.VirtualFree(memory, 0, win.MEM_RELEASE)
}


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

Fullscreen :: proc(window: win.HWND) {
	w := win.GetSystemMetrics(win.SM_CXSCREEN)
	h := win.GetSystemMetrics(win.SM_CYSCREEN)

	style := win.GetWindowLongW(window, win.GWL_STYLE)
	win.SetWindowLongW(window, win.GWL_STYLE, style & ~i32(win.WS_OVERLAPPEDWINDOW))

	win.SetMenu(window, nil)
	win.SetWindowPos(window, win.HWND_TOPMOST, 0, 0, w, h, win.SWP_NOZORDER | win.SWP_FRAMECHANGED)
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
		win.WS_OVERLAPPEDWINDOW | win.WS_VISIBLE,
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
	ok = true
	return
}

GetMouse :: proc() -> [2]i32 {
	pt: win.POINT
	if ok := win.GetCursorPos(&pt); !ok do fmt.println("")
	return {pt.x, pt.y}
}
