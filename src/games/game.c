#include "../engine.c"

struct GameState {
    Arena  scene;
    Camera cam;
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
    ClearScreen((v4){0.4, 0.2, 0.3, 1.0});
    for (size_t i = 0; i < 10; i++) {
        f32 v = 0.1 + 0.1 * i;
        if (Time() % 1000 > i * 100)
            DrawRectangle((Rect){120 + 20 * i + sin(Time() * 0.001) * 100, 40 + 20 * i, 200, 100},
                          (v4){v, v - 0.3, v, v}, 20);
    }
}