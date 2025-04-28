#include "games/game.c"

i32 main() {
    E = (EngineCtx *)malloc(128 * 1'000'000);

    EngineLoadGame(Setup, Init, Update, Draw);
    EngineInit();
    while (EngineIsRunning()) EngineUpdate();
    EngineShutdown();

    free(E);
}
