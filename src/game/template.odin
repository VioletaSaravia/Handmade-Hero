package game

import e "../engine"

import fmt "core:fmt"


GameState :: struct {
	player_pos: [2]f32,
}

@(export)
GameInit :: proc() {
	// TODO Split in init and setup. e.Settings goes on setup
	e.Settings = {
		name       = "Test",
		resolution = {800, 600},
	}
}

@(export)
GameUpdate :: proc() {
	fmt.println("HI!!")
}
