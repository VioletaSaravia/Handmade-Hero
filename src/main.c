#include "game.c"

i32 main() {
    MemRegion memory = NewMemRegion(sizeof(EngineCtx) + sizeof(GameState));

    E = (EngineCtx *)BufferAlloc(&memory, sizeof(EngineCtx));
    S = (GameState *)BufferAlloc(&memory, sizeof(GameState));

    GameLoad(Setup, Init, Update, Draw);

    GameEngineInit();
    while (GameIsRunning()) GameEngineUpdate();
    GameEngineShutdown();

    FREE(memory.data);
}
