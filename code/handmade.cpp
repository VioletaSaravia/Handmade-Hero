#include "handmade.h"

struct GameState {
    f64 delta;
    f64 testPitch;
    v2  pos[2];
};

extern "C" GAME_INIT(game_init) {
    GameState *state = (GameState *)memory->permStore;

    *state = {
        .testPitch = 220.0,
        .pos       = {{400, 300}},
    };
}

extern "C" GAME_UPDATE(game_update) {
    Assert(sizeof(memory->permStore) >= sizeof(GameState));
    GameState *state = (GameState *)memory->permStore;

    if (input->keyboard.keys['D'] >= ::Pressed) {
        state->pos[0].x += 100.0 * delta;
    }
    if (input->keyboard.keys['A'] >= ::Pressed) {
        state->pos[0].x -= 100.0 * delta;
    }
    if (input->keyboard.keys['W'] >= ::Pressed) {
        state->pos[0].y -= 100.0 * delta;
    }
    if (input->keyboard.keys['S'] >= ::Pressed) {
        state->pos[0].y += 100.0 * delta;
    }
}