#include "../win32/engine.c"

struct GameState {
    Camera  cam;
    VAO     test;
    Sound   testSound;
    Texture font;
    Arena   scene;
    v4      bgColor;
};

export void Setup() {
    E->Settings = (GameSettings){
        .name    = (cstr)L"Test Game",
        .version = "0.2",
    };
}

export void Init() {
    S->testSound = LoadSound("data\\test.wav", ONESHOT);
}

export void Update() {
    S->bgColor = (v4){0.8, 0.2, 0.3, 1.0};

    if (Input()->keys['P'] == JustPressed) PlaySound(S->testSound);
}

export void Draw() {
    ClearScreen(S->bgColor);
    for (size_t i = 0; i < 10; i++) {
        f32 v = 0.1 + 0.1 * i;
        if (Time() % 1000 > i * 100)
            DrawRectangle(
                (Rect){120 + 20 * i /*+ sin(Time() * 0.001) * 100*/, 40 + 20 * i, 200, 100},
                (v4){v, v, v, v}, 20);
    }
}