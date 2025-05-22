#include "../engine.c"

struct GameState {
    Arena  scene;
    Camera cam;
    bool   holding;
    v2     pos;
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

export void Init() {}

export void Update() {
    if (GetMouseButton(BUTTON_LEFT) == JustPressed &&
        V2InRect(Mouse(), (Rect){S->pos, (v2){200, 300}}))
        S->holding = true;
    if (GetMouseButton(BUTTON_LEFT) == JustReleased) S->holding = false;

    if (S->holding) {
        S->pos.x += MouseDir().x;
        S->pos.y += MouseDir().y;
    }
}

export void Draw() {
    DrawRectangle((Rect){S->pos.x, S->pos.y, 200, 300}, 0, WHITE, 0);
    persist bool bla = false;
    GuiToggle((Rect){50, 50, 50, 50}, &bla);
}