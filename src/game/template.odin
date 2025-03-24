package game

import e "../engine"

import fmt "core:fmt"

State: ^GameState
GameState :: struct {
	player_pos: [2]f32,
}

@(export)
GameSetup :: proc() {
	e.Settings = {
		name       = "Test",
		resolution = {800, 600},
	}
}

@(export)
GameInit :: proc() {
	// TODO Split in init and setup. e.Settings goes on setup
	State = auto_cast e.Alloc(size_of(GameState))
}

@(export)
GameUpdate :: proc() {
	fmt.println("HI!!")
}
