package engine

import intrinsics "base:intrinsics"
import "core:fmt"
import "core:mem"
import win "core:sys/windows"

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

Settings: GameSettings

Running: ^bool

@(export)
GameIsRunning :: proc() -> bool {return Running^}

Memory: [^]byte
Size: uint
Head: uint

Alloc :: proc(size: uint) -> (result: rawptr) {
	assert(Head + size < Size)
	result = auto_cast Memory[Head:]
	Head += size

	return
}

Alloc2 :: proc(t: $T) -> (result: ^T) {
	size: uint = auto_cast size_of(t)
	assert(Head + size < Size)
	result = auto_cast Memory[Head:]
	Head += size
	return
}

@(export)
GameGetMemory :: proc() -> rawptr {return Memory}

@(export)
GameReloadMemory :: proc(memory: rawptr) {Memory = auto_cast memory}


InitMemory :: proc(size: uint) -> rawptr {
	Size = size
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

	state.delta = GetSecondsElapsed(Window.perf_count_freq, state.last_counter, GetWallClock())
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

	fmt.println("Delta:", state.delta, "MSPF:", ms_per_frame, "MS behind:", ms_behind, "FPS:", fps)

	state.last_counter = GetWallClock()
	state.last_cycle_count = end_cycle_count
}

Audio: ^AudioBuffer
Input: ^InputBuffer
Timing: ^TimingBuffer
Window: ^WindowBuffer

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
	if Memory = auto_cast (InitMemory(Settings.memory)); Memory == nil do return

	Running = auto_cast Alloc(size_of(bool))
	Running^ = true
	Input = auto_cast Alloc(size_of(InputBuffer))
	Window = auto_cast Alloc(size_of(WindowBuffer))
	Timing = auto_cast Alloc(size_of(TimingBuffer))
	// Audio = auto_cast Alloc(AudioBuffer)

	ok: bool

	if Window^, ok = InitWindow(); !ok do return
	if ok = InitOpenGL(Window.window); !ok do return
	// if Audio^, ok = InitAudio(); !ok do return
	Timing^, Window.perf_count_freq = InitTiming(Window.refresh_rate)

	if Settings.fullscreen do Fullscreen(Window.window)
	GameProcs.Init()
}

@(export)
GameEngineUpdate :: proc() {
	ProcessKeyboard(Input)
	ProcessControllers(Input)
	ProcessMouse(Input)

	GameProcs.Update()

	TimeAndRender(Timing, Window)
}
