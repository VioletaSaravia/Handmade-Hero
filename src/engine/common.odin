package engine

import intrinsics "base:intrinsics"
import "core:fmt"
import "core:mem"
import win "core:sys/windows"
import gl "vendor:OpenGL"

v2 :: [2]f32
v3 :: [3]f32
v2i :: [2]i32

GameSettings :: struct {
	name:       string,
	version:    string,
	resolution: v2i,
	fullscreen: bool,
	memory:     uint,
}


@(export)
GameIsRunning :: proc() -> bool {return Mem.Running}


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
	target_spf := 1.0 / (f64(refresh_rate) / 2.0)
	result = {
		target_spf           = target_spf,
		last_counter         = InitPerformanceCounter(&perf_count_freq),
		last_cycle_count     = intrinsics.read_cycle_counter(),
		delta                = target_spf,
		desired_scheduler_ms = 1,
		granular_sleep_on    = auto_cast win.timeBeginPeriod(1),
	}

	return
}

GetWallClock :: proc() -> i64 {
	result: win.LARGE_INTEGER
	win.QueryPerformanceCounter(&result)
	return auto_cast result
}

InitPerformanceCounter :: proc(freq: ^i64) -> i64 {
	result: win.LARGE_INTEGER
	win.QueryPerformanceCounter(&result)
	freq^ = i64(result)

	return GetWallClock()
}

GetSecondsElapsed :: proc(perf_count_freq, start, end: i64) -> (result: f64) {
	result = f64(end - start) / f64(perf_count_freq)
	return
}

TimeAndRender :: proc(state: ^TimingBuffer, screen: ^WindowBuffer) {
	end_cycle_count := intrinsics.read_cycle_counter()
	cycles_elapsed := end_cycle_count - state.last_cycle_count
	megacycles_per_frame := f64(cycles_elapsed) / (1000.0 * 1000.0)

	state.delta = GetSecondsElapsed(Mem.Window.perf_count_freq, state.last_counter, GetWallClock())
	// for state.delta < state.target_spf {
	// 	if state.granular_sleep_on do win.Sleep(u32(1000.0 * (state.target_spf - state.delta)))
	// 	state.delta = GetSecondsElapsed(Window.perf_count_freq, state.last_counter, GetWallClock())
	// }

	ms_per_frame := state.delta * 1000.0
	ms_behind := (state.delta - state.target_spf) * 1000.0
	fps := f64(screen.perf_count_freq) / f64(GetWallClock() - state.last_counter)

	GameProcs.Draw()

	win.SwapBuffers(screen.dc)
	win.ReleaseDC(screen.window, screen.dc)

	// fmt.println("Delta:", state.delta, "MSPF:", ms_per_frame, "MS behind:", ms_behind, "FPS:", fps)

	state.last_counter = GetWallClock()
	state.last_cycle_count = end_cycle_count
}

Memory :: struct {
	Settings:    GameSettings,
	Running:     bool,
	Audio:       AudioBuffer,
	Input:       InputBuffer,
	Timing:      TimingBuffer,
	Window:      WindowBuffer,
	// Audio:      AudioBuffer,
	Shaders:     [8]Shader,
	DefaultRect: Rectangle,
	GameMemory:  [1]byte, // TODO(violeta): Mmmhh
}

Mem: ^Memory

@(export)
GameGetMemory :: proc() -> rawptr {return Mem}

@(export)
GameReloadMemory :: proc(memory: rawptr) {
	Mem = auto_cast memory
	gl.load_up_to(4, 6, win.gl_set_proc_address)
}

GameProcs: struct {
	Setup:  proc(),
	Init:   proc(),
	Update: proc(),
	Draw:   proc(),
}

@(export)
GameLoad :: proc(setup, init, update, draw: proc()) {
	GameProcs.Setup = setup
	GameProcs.Init = init
	GameProcs.Update = update
	GameProcs.Draw = draw
}

Settings :: proc(settings: GameSettings) {
	if Mem = auto_cast (InitMemory(settings.memory + size_of(Memory))); Mem == nil do return
	Mem.Running = true
	Mem.Settings = settings
}


@(export)
GameEngineInit :: proc() {
	GameProcs.Setup()

	ok: bool
	if Mem.Window, ok = InitWindow(); !ok do return
	if ok = InitOpenGL(Mem.Window.window); !ok do return
	Mem.DefaultRect = NewRectangle()
	// if Audio^, ok = InitAudio(); !ok do return
	Mem.Timing, Mem.Window.perf_count_freq = InitTiming(Mem.Window.refresh_rate)

	if Mem.Settings.fullscreen do Fullscreen(Mem.Window.window)

	if default_shader, ok := NewShader("", ""); ok {
		Mem.Shaders[0] = default_shader
	}
	GameProcs.Init()
}

@(export)
GameEngineUpdate :: proc() {
	ProcessKeyboard(&Mem.Input)
	ProcessControllers(&Mem.Input)
	ProcessMouse(&Mem.Input)

	for &s in Mem.Shaders {
		if err := ReloadShader(&s); err != nil {
			fmt.println("Shader Reload Error:", err)
		}
	}


	GameProcs.Update()

	TimeAndRender(&Mem.Timing, &Mem.Window)
}
