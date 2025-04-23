#include "win32.c"

typedef struct {
    Camera  cam;
    VAO     test;
    Texture font;
    Arena   alloc;
    cstr    ohno;
    i32     ohyes;
} GameState;
GameState *S;

export void Setup() {
    E->Settings = (GameSettings){
        .name       = (cstr)L"Test Game",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
        .scale      = 1,
    };
    S = (GameState *)((u8 *)EngineGetMemory() + sizeof(EngineCtx));
    if (!E->usedMemory) E->usedMemory = sizeof(EngineCtx) + sizeof(GameState); // TODO move
}

export void Init() {}

export void Update() {}

export void Draw() {
}