#include "win32.h"
#define DLL_PATH "build\\game.dll"

bool CopyDLL(cstr to) {
    return true;
}

typedef struct {
    u64   modTime;
    i32   version;
    void *lib;

    void (*Setup)();
    void (*Init)();
    void (*Update)();
    void (*Draw)();
} GameApi;

GameApi LoadApi(i32 vers) {
    u64 modTime = GetLastWriteTime(DLL_PATH);
    if (modTime == 0) return (GameApi){0};

    GameApi result = {0};

    char game_dll_name[256];
    snprintf(game_dll_name, sizeof(game_dll_name), "game_%d.dll", vers);
    if (!CopyDLL(game_dll_name)) {
        return (GameApi){0};
    }

    bool ok = initialize_symbols(&result, game_dll_name, "Game", "lib");
    if (!ok) {
        printf("Failed initializing symbols: %s\n", last_dynlib_error());
        return (GameApi){0};
    }

    result.version = vers;
    result.modTime = modTime;

    return result;
}

void UnloadApi(GameApi *api) {}

i32 main() {
    i32     apiVersion = 0;
    GameApi game       = LoadApi(apiVersion++);

    srand((u32)time(0));

    MemRegion buffer = NewMemRegion(sizeof(EngineCtx)); //+ sizeof(GameState));
    // E                = (EngineCtx *)BufferAlloc(&buffer, sizeof(EngineCtx));
    // S             = (GameState *)BufferAlloc(&buffer, sizeof(GameState));

    GameLoad(game.Setup, game.Init, game.Update, game.Draw);

    GameEngineInit();
    while (GameIsRunning()) {
        GameEngineUpdate();

        u64 dllMod = GetLastWriteTime(DLL_PATH);
        if (game.modTime == dllMod) continue;

        GameApi new = LoadApi(apiVersion);
        void *old   = GameGetMemory();

        game = new;
        GameReloadMemory(old);
        GameLoad(game.Setup, game.Init, game.Update, game.Draw);

        apiVersion++;
    }

    GameEngineShutdown();

    FREE(buffer.data);
}