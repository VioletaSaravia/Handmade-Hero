#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define DLL_PATH "build\\debug\\game"

typedef struct GameApi {
    HMODULE  lib;
    uint64_t writeTime;
    int32_t  version;

    void (*Setup)();
    void (*Init)();
    void (*Update)();
    void (*Draw)();
    void (*EngineLoadGame)(void (*setup)(), void (*init)(), void (*update)(), void (*draw)());
    void (*EngineInit)();
    void (*EngineUpdate)();
    void (*EngineShutdown)();
    bool (*EngineIsRunning)();
    void *(*EngineGetMemory)();
    void (*EngineReloadMemory)(void *memory);
    uint64_t (*GetLastWriteTime)(char *file);
} GameApi;

GameApi LoadApi(void *memory, int32_t version) {
    char dllBuf[7 + sizeof(DLL_PATH)];
    snprintf(dllBuf, sizeof(dllBuf), DLL_PATH "%02d.dll", version);
    version++;

    while (!CopyFile(DLL_PATH ".dll", dllBuf, false)) {
        DWORD err = GetLastError();
        if (err != 32) {
            printf("[Error] [%s] Couldn't copy file, code %i. Aborting\n", __func__, err);
            return (GameApi){0};
        }
    }

    HMODULE lib = LoadLibraryA(dllBuf);
    if (!lib) return (GameApi){0};

#pragma warning(push)
#pragma warning(disable : 4113)
#pragma warning(disable : 4133)
#pragma warning(disable : 4047)
    GameApi api = (GameApi){
        .lib                = lib,
        .writeTime          = 0,
        .version            = version,
        .Setup              = GetProcAddress(lib, "Setup"),
        .Init               = GetProcAddress(lib, "Init"),
        .Update             = GetProcAddress(lib, "Update"),
        .Draw               = GetProcAddress(lib, "Draw"),
        .EngineLoadGame     = GetProcAddress(lib, "EngineLoadGame"),
        .EngineInit         = GetProcAddress(lib, "EngineInit"),
        .EngineUpdate       = GetProcAddress(lib, "EngineUpdate"),
        .EngineShutdown     = GetProcAddress(lib, "EngineShutdown"),
        .EngineIsRunning    = GetProcAddress(lib, "EngineIsRunning"),
        .EngineGetMemory    = GetProcAddress(lib, "EngineGetMemory"),
        .EngineReloadMemory = GetProcAddress(lib, "EngineReloadMemory"),
        .GetLastWriteTime   = GetProcAddress(lib, "GetLastWriteTime"),
    };
#pragma warning(pop)

    api.writeTime = api.GetLastWriteTime(dllBuf);

    api.EngineReloadMemory(memory);
    api.EngineLoadGame(api.Setup, api.Init, api.Update, api.Draw);

    return api;
}

void ReloadApi(GameApi *api) {
    uint64_t latestWriteTime = api->GetLastWriteTime(DLL_PATH ".dll");
    if (latestWriteTime <= api->writeTime) return;

    Sleep(200);
    GameApi newApi = LoadApi(api->EngineGetMemory(), api->version);
    if (!newApi.lib) {
        printf("[Error] [Debug] Couldn't reload dll\n");
        api->writeTime = latestWriteTime;
        return;
    }
    *api = newApi;
}

int32_t main() {
    GameApi api =
        LoadApi(VirtualAlloc(0, 128 * 1'000'000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), 0);
    if (!api.lib) {
        printf("[Fatal] [Debug] Couldn't load dll\n");
        return;
    }

    api.EngineInit();
    while (api.EngineIsRunning()) {
        api.EngineUpdate();
        ReloadApi(&api);
    }
    api.EngineShutdown();
}