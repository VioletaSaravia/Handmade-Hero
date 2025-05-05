#include "../engine.c"

struct GameState {
    Arena   scene;
    Camera  cam;
    Texture precise, preciseBold;
    Poly    poly;
    v2      wPos;
    bool    moving;
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

export void Update() {
    if (GetKey(KEY_E) >= Pressed) S->cam.rotation -= 2 * Delta();
    if (GetKey(KEY_Q) >= Pressed) S->cam.rotation += 2 * Delta();
    if (GetKey(KEY_A) >= Pressed) S->cam.pos.x -= 200 * Delta();
    if (GetKey(KEY_D) >= Pressed) S->cam.pos.x += 200 * Delta();
    if (GetKey(KEY_W) >= Pressed) S->cam.pos.y += 200 * Delta();
    if (GetKey(KEY_S) >= Pressed) S->cam.pos.y -= 200 * Delta();
    if (GetKey(KEY_F) >= Pressed) S->cam.zoom += 1 * Delta();
    if (GetKey(KEY_C) >= Pressed) S->cam.zoom -= 1 * Delta();
    if (GetKey(KEY_R) >= JustPressed) S->cam = (Camera){0};

    S->moving = GetMouseButton(BUTTON_LEFT) >= Pressed &&
                V2InRect(S->cam.pos, (Rect){.pos = S->wPos, .size = (v2){200, 50}});

    if (S->moving) S->wPos = v2Add(S->wPos, MouseDir());
}

export void Draw() {
    ClearScreen((v4){0.4, 0.2, 0.3, 1.0});
    CameraBegin(S->cam);

    CameraEnd();

    DrawRectangle((Rect){S->wPos.x, S->wPos.y, 180, 250}, 0, BLACK, 0.2, true, 2);
    GuiButton((Rect){S->wPos.x, S->wPos.y, 16, 16});
    GuiButton((Rect){S->wPos.x, S->wPos.y, 16, 16});
    GuiButton((Rect){S->wPos.x, S->wPos.y, 16, 16});
}