package engine

v2 :: [2]f32
v3 :: [3]f32
v2i :: [2]i32

GameSettings :: struct {
	name:       string,
	resolution: v2i,
}

Settings: GameSettings

Running: bool = true

@(export)
GameIsRunning :: proc() -> bool {return Running}

Memory: [^]byte
Size: u32
Head: u32

Alloc :: proc(size: u32) -> (result: [^]byte) {
	assert(Head + size < Size)
	result = Memory[Head:]
	Head += size

	return
}

@(export)
GameGetMemory :: proc() -> rawptr {return Memory}

@(export)
GameReloadMemory :: proc(memory: rawptr) {Memory = auto_cast memory}
