package main

import game "../game"
import engine "../win32"

main :: proc() {
	engine.GameLoad(game.GameSetup, game.GameInit, game.GameUpdate, game.GameDraw)
	engine.GameEngineInit()
	for engine.GameIsRunning() do engine.GameEngineUpdate()
	engine.GameEngineShutdown()
}
