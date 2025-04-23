#include "../win32/engine.c"

typedef struct {
    Camera  cam;
    VAO     test;
    Texture font;
    Arena   alloc;
    v4      bgColor;
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

export void Init() {
}

export void Update() {
    S->bgColor = (v4){0.1, 0.3, 0.8, 1.0};
}

export void Draw() {
    ClearScreen(S->bgColor);
}