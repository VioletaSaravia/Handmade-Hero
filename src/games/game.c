#include "../engine.c"

struct GameState {
    Arena   scene;
    Camera  cam;
};

export void Setup() {
    *Settings() = (GameSettings){
        .name       = "Test",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
    };
}

export void Init() {}

export void Update() {}

export void Draw() {
    DrawRectangle((Rect){0, 0, 100, 100}, 0, WHITE, 0);
    DrawLine((v2){0, 0}, (v2){300, 300}, WHITE, 10);
}