#include "../engine.c"

struct GameState {
    Arena   scene;
    Camera  cam;
    Texture precise, preciseBold;
    Poly    poly;
};

export void Setup() {
    *Settings() = (GameSettings){
        .name       = "Test",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
    };
}

export void Init() {
    S->scene       = NewArena(Memory(), 1024);
    S->precise     = NewTexture("data\\precise3x.png");
    S->preciseBold = NewTexture("data\\precise3x_bold.png");

    S->poly = (Poly){.count = 10, .verts = Alloc(&S->scene, 10 * sizeof(v2))};
    for (u32 i = 0; i < S->poly.count; i++) {
        S->poly.verts[i] = (v2){.x = SDL_rand(640), .y = SDL_rand(360)};
    }
}

export void Update() {}

export void Draw() {
    ClearScreen((v4){0.4, 0.2, 0.3, 1.0});

    for (size_t i = 0; i < 10; i++) {
        f32 v = 0.1 + 0.1 * i;
        if (Time() % 1000 > i * 100)
            DrawRectangle((Rect){120 + 20 * i + sin(Time() * 0.001) * 100, 40 + 20 * i, 200, 100},
                          0, (v4){v, v - 0.3, v, v}, 0.2, false, 20);
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

    persist i32 counter = 0;
    GuiCounter((v2){300, 10}, 20, &counter);

    DrawRectangle((Rect){100, 10, 50, 20}, 0, WHITE, 4, false, 0);

    DrawCircle((v2){370, 225}, 30, WHITE, true, 5);
    DrawCircle((v2){410, 225}, 30, BLACK, false, 0);
    DrawRectangle((Rect){425, 200, 50, 50}, PI * 0.25 + Time() * -0.002, WHITE, 0.15, true, 5);
    DrawRectangle((Rect){455, 200, 50, 50}, PI * 0.25 + Time() * -0.002, BLACK, 0.5, true, 4);
    DrawHex((v2){510, 225}, 30, PI + Time() * 0.002, WHITE, 0, true, 5);
    DrawHex((v2){540, 225}, 30, Time() * 0.002, BLACK, 0, false, 2);
}