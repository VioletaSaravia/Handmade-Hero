package engine

import intrinsics "base:intrinsics"
import win "core:sys/windows"


InitMemory :: proc(size: uint) -> rawptr {
	return win.VirtualAlloc(nil, size, win.MEM_COMMIT | win.MEM_RESERVE, win.PAGE_READWRITE)
}

TimingBuffer :: struct {
	target_spf, delta:    f64,
	last_counter:         i64,
	last_cycle_count:     i64,

	// Used by sleep()
	desired_scheduler_ms: u32,
	granular_sleep_on:    bool,
}

InitTiming :: proc(refresh_rate: u32) -> (result: TimingBuffer, perf_count_freq: i64) {
	result = {
		target_spf           = 1.0 / (f64(refresh_rate) / 2.0),
		last_counter         = InitPerformanceCounter(&perf_count_freq),
		last_cycle_count     = intrinsics.read_cycle_counter(),
		delta                = Timing.target_spf,
		desired_scheduler_ms = 1,
		granular_sleep_on    = win.timeBeginPeriod(1) != 0,
	}

	return
}

GetWallClock :: proc() -> i64 {
	result: win.LARGE_INTEGER
	win.QueryPerformanceCounter(&result)
	return i64(result)
}

InitPerformanceCounter :: proc(freq: ^i64) -> i64 {
	result: win.LARGE_INTEGER
	win.QueryPerformanceCounter(&result)
	freq^ = i64(result)

	return GetWallClock()
}

GetSecondsElapsed :: proc(start, end: i64) -> f64 {
	return f64(end - start) / f64(Window.perf_count_freq)
}

TimeAndRender :: proc(state: ^TimingBuffer, screen: ^WindowBuffer) {
	end_cycle_count := intrinsics.read_cycle_counter()
	cycles_elapsed := end_cycle_count - state.last_cycle_count
	megacycles_per_frame := f64(cycles_elapsed) / (1000.0 * 1000.0)

	state.delta = GetSecondsElapsed(state.last_counter, GetWallClock())
	for state.delta < state.target_spf {
		if state.granular_sleep_on do win.Sleep(u32(1000.0 * (state.target_spf - state.delta)))
		state.delta = GetSecondsElapsed(state.last_counter, GetWallClock())
	}

	ms_per_frame := state.delta * 1000.0
	ms_behind := (state.delta - state.target_spf) * 1000.0
	fps := f64(screen.perf_count_freq) / f64(GetWallClock() - state.last_counter)

	Draw()

	win.SwapBuffers(screen.dc)
	win.ReleaseDC(screen.window, screen.dc)

	state.last_counter = GetWallClock()
	state.last_cycle_count = end_cycle_count
}


Window: WindowBuffer
Audio: AudioBuffer
Input: InputBuffer
Timing: TimingBuffer


GameProcs: struct {
	Init:   proc(),
	Update: proc(),
}

@(export)
GameSetProcs :: proc(init, update: proc()) {
	GameProcs.Init = init
	GameProcs.Update = update
}

@(export)
GameEngineInit :: proc() {
	ok: bool
	Memory = (InitMemory(64 * 1000 * 1000))
	if Memory == nil do return

	Window, ok = InitWindow()
	if !ok do return

	ok = InitOpenGL(Window.window)
	if !ok do return

	Audio, ok = InitAudio()
	if !ok do return

	Timing, Window.perf_count_freq = InitTiming(Window.refresh_rate)

	GameProcs.Init()
}

@(export)
GameEngineUpdate :: proc() {
	ProcessKeyboard()
	ProcessControllers()

	GameProcs.Update()

	TimeAndRender(&Timing, &Window)
}
