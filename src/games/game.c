#include "../win32/engine.c"

struct GameState {
    Camera  cam;
    VAO     test;
    Texture font;
    Arena   scene;
    v4      bgColor;
};

export void Setup() {
    E->Settings = (GameSettings){
        .name       = (cstr)L"Test Game",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
        .scale      = 1,
    };
}

export void Init() {}

export void Update() {
    // S          = (GameState *)(((u8 *)EngineGetMemory()) + sizeof(EngineCtx));
    S->bgColor = (v4){0.4, 0.2, 0.3, 1.0};
}

export void Draw() {
    ClearScreen(S->bgColor);
}