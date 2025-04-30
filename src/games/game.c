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

    persist f32 levels[28] = {0};
    for (u32 i = 0; i < 21; i++) {
        if (i / 14)
            GuiSliderH((v2){340, 55 + (i - 14) * 20}, 270, &levels[i]);
        else
            GuiSliderV((v2){40 + i * 20, 50}, 200, &levels[i]);

        if (Time() % 400 < 10) levels[i] = SDL_randf();
    }

    persist bool toggles[10] = {0};
    for (u32 i = 0; i < 10; i++) {
        GuiToggle((Rect){(i % 5) * 30 + 150, 275 + (i / 5) * 30, 25, 15}, &toggles[i]);
        GuiButton((Rect){(i % 5) * 30 + 350, 275 + (i / 5) * 30, 25, 15});

        if (Time() % 300 < 10) toggles[i] = SDL_rand(2);
    }

    for (i32 i = 0; i < 5; i++) {
        DrawNgon((Rect){200 + i * 60, 125, 300, 200}, (v4){0.1, 0.02 * i, 0.07 * i, 1}, 20, 6,
                 ((Time() + i * 1000) / 10 * (i / 10 + 1)) % 360);
    }
}