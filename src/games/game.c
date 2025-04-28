#include "../engine.c"

struct GameState {
    Camera  cam;
    VAO     test;
    Sound   testSound;
    Texture font;
    Texture ship;
    Arena   scene;
    v4      bgColor;
};

export void Setup() {
    *Settings() = (GameSettings){
        .name              = (cstr)L"Test Game",
        .version           = "0.2",
        .defaultResolution = (v2i){640, 360},
        .scale             = 1,
    };
}

export void Init() {
    S->testSound = NewSound("data\\test.wav", 1);
    SoundSetPan(S->testSound, 0.9f);
    SoundSetVol(S->testSound, 0.1f);
    SoundPlay(S->testSound);
}

export void Update() {
    S->bgColor = (v4){0.4, 0.2, 0.3, 1.0};
}

export void Draw() {
    ClearScreen(S->bgColor);
    for (size_t i = 0; i < 10; i++) {
        f32 v = 0.1 + 0.1 * i;
        if (Time() % 1000 > i * 100)
            DrawRectangle(
                (Rect){120 + 20 * i /*+ sin(Time() * 0.001) * 100*/, 40 + 20 * i, 200, 100},
                (v4){v, v - 0.3, v, v}, 20);
    }
}