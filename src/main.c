#include "games/game.c"

i32 main() {
    E = (EngineCtx *)RAW_ALLOC(128 * 1'000'000);

    EngineLoadGame(Setup, Init, Update, Draw);
    Setup();
    EngineInit();
    while (EngineIsRunning()) EngineUpdate();
    EngineShutdown();

    RAW_FREE(E);
}
