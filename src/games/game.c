#include "../engine.c"

struct GameState {
    Arena  scene;
    Camera cam;
    bool   holding;
    v2     pos;
    Text   text;
};

enum Action {
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_ACCEPT,
    ACTION_CANCEL,
    ACTION_COUNT,
};

global i32 Keymap[ACTION_COUNT] = {
    [ACTION_UP] = 283,
};

export void Setup() {
    *Settings() = (GameSettings){
        .name       = "Test",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
    };
}

export void Init() {
    S->text = NewText("Hello. This is a sentence. Bye!", "data\\jetbrains.ttf", 12, 15);
}

export void Update() {}

export void Draw() {
    DrawText(S->text, (v2){100, 100});
}