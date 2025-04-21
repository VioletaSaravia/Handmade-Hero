#include "win32.c"

typedef struct {
    Camera cam;
} GameState;
GameState *S;

export void Setup() {
    E->Settings = (GameSettings){
        .name       = (cstr)L"Test Game",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
        .scale      = 2,
    };
    S = (GameState *)((u8 *)EngineGetMemory() + sizeof(EngineCtx));
}

export void Init() {}

export void Update() {}

export void Draw() {
    DrawText("Hello. This is the framework I'm developing for making games. This is how I write "
             "text in it. All handmade, no libraries. The only fonts supported are mono, since ttf "
             "is too difficult. But it should be fine. I do have word wrapping, though, that's "
             "nice. I sure do hope something good comes out of this!!",
             (v2){8, 8}, 50, 2);
}
