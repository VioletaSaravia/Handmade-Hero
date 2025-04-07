package main

import app "../apps"
import engine "../engine"

main :: proc() {
	engine.GameLoad(app.GameSetup, app.GameInit, app.GameUpdate, app.GameDraw)
	engine.GameEngineInit()
	for engine.GameIsRunning() do engine.GameEngineUpdate()
	engine.GameEngineShutdown()
}
