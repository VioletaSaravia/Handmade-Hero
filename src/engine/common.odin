package engine

import "core:fmt"
import "core:mem"
import gl "vendor:OpenGL"

DATA :: "../../data/" // TODO(violeta): arruina el linter: when ODIN_DEBUG else "data/"
// DATAS := #load_directory("../../data/")

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

Shaders :: enum {
	Default,
	Tiled,
}

GraphicsBuffer :: struct {
	shaders:       [Shaders]Shader,
	post_shader:   PostShader,
	active_shader: u32,
	square_mesh:   Mesh,
	mouse:         Texture,
}

@(export)
GameGetMemory :: proc() -> rawptr {return Mem}

@(export)
GameIsRunning :: proc() -> bool {return Mem.Window.running}

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
	if err := ReloadShader(&Graphics().post_shader.shader); err != nil {
		fmt.println("Shader Reload Error:", err)
	}

	GameProcs.Update()
	TimeAndRender(&Mem.Timing, &Mem.Window)
}

@(export)
GameEngineShutdown :: proc() {
	ShutdownAudio(&Mem.Audio)
}
