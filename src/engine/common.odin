package engine

v2 :: [2]f32
v3 :: [3]f32

GameSettings :: struct {
	name:       string,
	resolution: [2]i32,
}

Settings: GameSettings

Running: bool = true

@(export)
GameIsRunning :: proc() -> bool {return Running}

Memory: rawptr

@(export)
GameGetMemory :: proc() -> rawptr {return Memory}

@(export)
GameReloadMemory :: proc(memory: rawptr) {Memory = memory}
