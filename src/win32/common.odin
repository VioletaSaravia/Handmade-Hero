package engine

import intrinsics "base:intrinsics"
import "core:fmt"
import "core:mem"
import win "core:sys/windows"
import gl "vendor:OpenGL"

DATA :: "../../data/" // TODO(violeta): arruina el linter: when ODIN_DEBUG else "data/"

Input :: proc() -> ^InputBuffer {return &Mem.Input}
Audio :: proc() -> ^AudioBuffer {return &Mem.Audio}
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
	fullscreen: bool,
	memory:     int,
}

TimingBuffer :: struct {
	delta, target_spf:    f32,
	last_counter:         i64,
	last_cycle_count:     i64,

	// Used by sleep()
	desired_scheduler_ms: u32,
	granular_sleep_on:    bool,
}

Shaders :: enum {
	Textured,
	Untextured,
	Tiled,
	Font,
}

GraphicsBuffer :: struct {
	shaders:     [Shaders]Shader,
	square_mesh: Mesh,
}

InitTiming :: proc(refresh_rate: u32) -> (result: TimingBuffer, perf_count_freq: i64) {
	target_spf := 1.0 / (f32(refresh_rate))
	result = {
		target_spf           = target_spf,
		last_counter         = InitPerformanceCounter(&perf_count_freq),
		last_cycle_count     = intrinsics.read_cycle_counter(),
		delta                = auto_cast target_spf,
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

	state.delta =
	auto_cast GetSecondsElapsed(Mem.Window.perf_count_freq, state.last_counter, GetWallClock())
	for state.delta < state.target_spf {
		if state.granular_sleep_on do win.Sleep(u32(1000.0 * (state.target_spf - state.delta)))
		state.delta =
		auto_cast GetSecondsElapsed(Mem.Window.perf_count_freq, state.last_counter, GetWallClock())
	}

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

Settings :: proc(settings: GameSettings) {
	ptr, _ := mem.alloc(settings.memory + size_of(Memory))
	Mem = auto_cast ptr
	Mem.Settings = settings
}

@(export)
GameEngineInit :: proc() {
	GameProcs.Setup()

	ok: bool
	if Mem.Window, ok = InitWindow(); !ok do return
	if ok = InitOpenGL(Mem.Window.window, &Mem.Settings); !ok do return
	Mem.Graphics.square_mesh = NewMesh(square_vertices[:], square_indices[:])
	if ok = InitAudio(&Mem.Audio); !ok do return
	Mem.Timing, Mem.Window.perf_count_freq = InitTiming(Mem.Window.refresh_rate)

	if Mem.Settings.fullscreen do Fullscreen(Mem.Window.window)

	if shader, ok := NewShader("", ""); ok {
		Mem.Graphics.shaders[.Untextured] = shader
	}
	if shader, ok := NewShader("", "textured.frag"); ok {
		Mem.Graphics.shaders[.Textured] = shader
	}
	if shader, ok := NewShader("", "tiled.frag"); ok {
		Mem.Graphics.shaders[.Tiled] = shader
	}
	if shader, ok := NewShader("", "font.frag"); ok {
		Mem.Graphics.shaders[.Font] = shader
	}

	GameProcs.Init()
}

@(export)
GameEngineUpdate :: proc() {
	ProcessKeyboard(&Mem.Input)
	ProcessControllers(&Mem.Input)
	ProcessMouse(&Mem.Input)

	when ODIN_DEBUG {
		if Input().keys[Key.F11] == .JustPressed do Fullscreen(Mem.Window.window)
		if Input().keys[Key.Esc] == .JustPressed do Mem.Window.running = false

		for k, i in Input().keys do if k != .Released {
			fmt.println("Key", i, "was", k)
		}

		for &s in Mem.Graphics.shaders {
			if err := ReloadShader(&s); err != nil {
				fmt.println("Shader Reload Error:", err)
			}
		}
	}

	GameProcs.Update()
	TimeAndRender(&Mem.Timing, &Mem.Window)
}

@(export)
GameEngineShutdown :: proc() {
	ShutdownAudio(&Mem.Audio)
}
