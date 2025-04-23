#include "game.c"

i32 main() {
    MemRegion memory = NewMemRegion(sizeof(EngineCtx) + sizeof(GameState));

    E = (EngineCtx *)BufferAlloc(&memory, sizeof(EngineCtx));
    S = (GameState *)BufferAlloc(&memory, sizeof(GameState));

    EngineLoadGame(Setup, Init, Update, Draw);
    Setup();
    EngineInit();
    while (EngineIsRunning()) EngineUpdate();
    EngineShutdown();

    FREE(memory.data);
}
