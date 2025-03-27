package main

import "core:c/libc"
import "core:dynlib"
import "core:fmt"
import "core:log"
import "core:mem"
import "core:os"
import "core:os/os2"
import fp "core:path/filepath"

GAME_DLL_PATH :: "game.dll"

CopyDLL :: proc(to: string) -> bool {
	copy_err := os2.copy_file(to, GAME_DLL_PATH)

	if copy_err != nil {
		fmt.printfln("Failed to copy " + GAME_DLL_PATH + " to {0}: %v", to, copy_err)
		return false
	}

	return true
}

GameAPI :: struct {
	// Reload Data
	mod_time:     os.File_Time,
	version:      i32,
	lib:          dynlib.Library,

	// Procs
	EngineInit:   proc(),
	EngineUpdate: proc(),
	Load:         proc(setup, init, update, draw: proc()),
	Setup:        proc(),
	Init:         proc(),
	Update:       proc(),
	Draw:         proc(),
	ReloadMemory: proc(memory: rawptr),
	IsRunning:    proc() -> bool,
	GetMemory:    proc() -> rawptr,
}


LoadAPI :: proc(api_version: i32) -> (api: GameAPI, ok: bool) {
	mod_time, mod_time_error := os.last_write_time_by_name(GAME_DLL_PATH)
	if mod_time_error != os.ERROR_NONE {
		fmt.printfln(
			"Failed getting last write time of " + GAME_DLL_PATH + ", error code: {1}",
			mod_time_error,
		)
		return
	}

	game_dll_name := fmt.tprintf("game_{0}.dll", api_version)
	CopyDLL(game_dll_name) or_return

	if _, ok = dynlib.initialize_symbols(&api, game_dll_name, "Game", "lib"); !ok {
		fmt.printfln("Failed initializing symbols: {0}", dynlib.last_error())
	}

	api.Load(api.Setup, api.Init, api.Update, api.Draw)
	api.version = api_version
	api.mod_time = mod_time
	ok = true

	return
}

UnloadAPI :: proc(api: ^GameAPI) {
	if api.lib != nil {
		if !dynlib.unload_library(api.lib) {
			fmt.printfln("Failed unloading lib: {0}", dynlib.last_error())
		}
	}

	if os.remove(fmt.tprintf("game_{0}.dll", api.version)) != nil {
		fmt.printfln("Failed to remove game_{0}.dll" + " copy", api.version)
	}
}

ResetTrackingAllocator :: proc(a: ^mem.Tracking_Allocator) -> (ok: bool = true) {
	for _, value in a.allocation_map {
		fmt.printf("%v: Leaked %v bytes\n", value.location, value.size)
		ok = false
	}

	mem.tracking_allocator_clear(a)
	return
}

main :: proc() {
	exe_path := os.args[0]
	exe_dir := fp.dir(string(exe_path), context.temp_allocator)
	if err := os.set_current_directory(exe_dir); err != nil {
		fmt.printf("Couldn't set current directory to %s", exe_dir)
	}

	context.logger = log.create_console_logger()

	default_allocator := context.allocator
	tracking_allocator: mem.Tracking_Allocator
	mem.tracking_allocator_init(&tracking_allocator, default_allocator)
	context.allocator = mem.tracking_allocator(&tracking_allocator)

	api_version: i32 = 0
	g, ok := LoadAPI(api_version)
	if !ok {
		fmt.println("Failed to load Game API")
		return
	}
	api_version += 1

	g.EngineInit()
	old_apis := make([dynamic]GameAPI, default_allocator)

	loop: for g.IsRunning() {
		g.EngineUpdate()

		game_dll_mod, game_dll_mod_err := os.last_write_time_by_name(GAME_DLL_PATH)
		if game_dll_mod_err != os.ERROR_NONE || g.mod_time == game_dll_mod do continue

		if new_api, ok := LoadAPI(api_version); ok {
			append(&old_apis, g)
			game_memory := g.GetMemory()
			g = new_api
			g.ReloadMemory(game_memory)

			api_version += 1
		}
	}

	if g.lib != nil && !dynlib.unload_library(g.lib) {
		fmt.printfln("Failed unloading lib: {0}", dynlib.last_error())
	}

	for api in old_apis {
		if !dynlib.unload_library(api.lib) {
			fmt.printfln("Failed unloading lib: {0}", dynlib.last_error())
		}
	}

	for i in 0 ..< 99 {
		if err := os.remove(fmt.aprintf("game_{0}.dll", i)); err != os.ERROR_NONE {
			fmt.println("DLL not deleted.")
		}
		if err := os.remove(fmt.aprintf("game_{0}.pdb", i)); err != os.ERROR_NONE {
			fmt.println("DLL not deleted.")
		}
	}
}
