package main

import engine "../engine"
import game "../game"

import "core:fmt"

main :: proc() {
	engine.GameLoad(game.GameSetup, game.GameInit, game.GameUpdate, game.GameDraw)
	engine.GameEngineInit()
	for engine.GameIsRunning() do engine.GameEngineUpdate()
}
