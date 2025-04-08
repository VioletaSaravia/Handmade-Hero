package engine

import intrinsics "base:intrinsics"
import "core:fmt"
import "core:mem"
import win "core:sys/windows"
import gl "vendor:OpenGL"

DATA :: "../../data/" // TODO(violeta): arruina el linter: when ODIN_DEBUG else "data/"

Input :: proc() -> ^InputBuffer {return &Mem.Input}
Audio :: proc() -> ^AudioBuffer {return &Mem.Audio}
Graphics :: proc() -> ^GraphicsBuffer {return &Mem.Graphics}
Delta :: proc() -> f32 {return Mem.Timing.delta}
GetMemory :: proc(offset: uint) -> rawptr {return auto_cast raw_data(Mem.GameMemory[:])}

Mem: ^Memory
Memory :: struct {
	Settings:   GameSettings,
	Input:      InputBuffer,
	Timing:     TimingBuffer,
	Window:     WindowBuffer,
	Audio:      AudioBuffer,
	Graphics:   GraphicsBuffer,
	GameMemory: [1]byte, // TODO(violeta): Mmmhh
}

GameSettings :: struct {
	name:       string,
	version:    string,
	resolution: [2]i32,
	memory:     int,
	mouse:      bool,
}

Settings :: proc(
	name: string,
	version: string,
	resolution: [2]i32 = {640, 480},
	memory: int = 0,
	mouse: bool = true,
) {
	ptr, _ := mem.alloc(memory + size_of(Memory))
	Mem = auto_cast ptr
	Mem.Settings = {name, version, resolution, memory, mouse}
}

TimingBuffer :: struct {
	delta, target_spf:    f32,
	last_counter:         i64,
	last_cycle_count:     i64,
	perf_count_freq:      i64,

	// Used by sleep()
	desired_scheduler_ms: u32,
	granular_sleep_on:    bool,
}

Shaders :: enum {
	Default,
	Tiled,
}

GraphicsBuffer :: struct {
	shaders:       [Shaders]Shader,
	active_shader: u32,
	square_mesh:   Mesh,
	mouse:         Texture,
}

InitTiming :: proc(refresh_rate: u32) -> (result: TimingBuffer) {
	target_spf := 1.0 / (f32(refresh_rate))
	perf_count_freq: i64
	result = {
		target_spf           = target_spf,
		last_counter         = InitPerformanceCounter(&perf_count_freq),
		last_cycle_count     = intrinsics.read_cycle_counter(),
		delta                = auto_cast target_spf,
		desired_scheduler_ms = 1,
		granular_sleep_on    = auto_cast win.timeBeginPeriod(1),
	}
	result.perf_count_freq = perf_count_freq

	return
}

GetWallClock :: proc() -> i64 {
	result: win.LARGE_INTEGER
	win.QueryPerformanceCounter(&result)
	return auto_cast result
}

InitPerformanceCounter :: proc(freq: ^i64) -> i64 {
	result: win.LARGE_INTEGER
	win.QueryPerformanceFrequency(&result)
	freq^ = i64(result)

	return GetWallClock()
}

GetSecondsElapsed :: proc(perf_count_freq, start, end: i64) -> f32 {
	return f32(end - start) / f32(perf_count_freq)
}

TimeAndRender :: proc(state: ^TimingBuffer, screen: ^WindowBuffer) {
	end_cycle_count := intrinsics.read_cycle_counter()
	cycles_elapsed := end_cycle_count - state.last_cycle_count
	megacycles_per_frame := f64(cycles_elapsed) / (1000.0 * 1000.0)

	state.delta = GetSecondsElapsed(state.perf_count_freq, state.last_counter, GetWallClock())
	for state.delta < state.target_spf {
		if state.granular_sleep_on do win.Sleep(u32(1000.0 * (state.target_spf - state.delta)))
		state.delta = GetSecondsElapsed(state.perf_count_freq, state.last_counter, GetWallClock())
	}

	ms_per_frame := state.delta * 1000.0
	ms_behind := (state.delta - state.target_spf) * 1000.0
	fps := f64(state.perf_count_freq) / f64(GetWallClock() - state.last_counter)

	ClearScreen()
	GameProcs.Draw()
	if Mem.Settings.mouse {
		UseShader(Graphics().shaders[.Default])
		SetUniform("res", GetResolution())
		SetUniform("pos", GetMouse())
		SetUniform("scale", 1.0)
		SetUniform("size", [2]f32{cast(f32)Graphics().mouse.w, cast(f32)Graphics().mouse.h})
		SetUniform("color", WHITE)

		UseTexture(Graphics().mouse)
		DrawMesh(Graphics().square_mesh)
	}

	win.SwapBuffers(screen.dc)
	win.ReleaseDC(screen.window, screen.dc)

	// fmt.println("Delta:", state.delta, "MSPF:", ms_per_frame, "MS behind:", ms_behind, "FPS:", fps)

	state.last_counter = GetWallClock()
	state.last_cycle_count = end_cycle_count
}


@(export)
GameGetMemory :: proc() -> rawptr {return Mem}

@(export)
GameIsRunning :: proc() -> bool {return Mem.Window.running}

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

@(export)
GameEngineInit :: proc() {
	GameProcs.Setup()

	ok: bool
	if Mem.Window, ok = InitWindow(); !ok do return
	if Mem.Graphics, ok = InitGraphics(Mem.Window.window, &Mem.Settings); !ok do return
	if ok = InitAudio(&Mem.Audio); !ok do return
	Mem.Timing = InitTiming(Mem.Window.refresh_rate)

	if !ODIN_DEBUG do Fullscreen(Mem.Window.window)

	GameProcs.Init()
}

@(export)
GameEngineUpdate :: proc() {
	ProcessKeyboard(&Mem.Input)
	ProcessControllers(&Mem.Input)
	ProcessMouse(&Mem.Input)

	when ODIN_DEBUG {
		if Input().keys[Key.F12] == .JustPressed do GameProcs.Init()
		if Input().keys[Key.F11] == .JustPressed do Fullscreen(Mem.Window.window)
		if Input().keys[Key.Esc] == .JustPressed do Mem.Window.running = false

		for k, i in Input().keys do if k != .Released {
			fmt.println("Key", i, "was", k)
		}
	}

	for &s in Mem.Graphics.shaders do if err := ReloadShader(&s); err != nil {
		fmt.println("Shader Reload Error:", err)
	}

	GameProcs.Update()
	TimeAndRender(&Mem.Timing, &Mem.Window)
}

@(export)
GameEngineShutdown :: proc() {
	ShutdownAudio(&Mem.Audio)
}
