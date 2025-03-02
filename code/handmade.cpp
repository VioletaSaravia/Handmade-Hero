#include "handmade.h"

internal void DrawCircle(ScreenBuffer *buffer, v2 pos, f32 size, u8 color[4]) {
    i32 stride = buffer->Width * buffer->BytesPerPixel;
    u8 *row    = (u8 *)buffer->Memory;
    for (i32 y = 0; y < buffer->Height; y++) {
        u32 *pixel = (u32 *)row;
        for (i32 x = 0; x < buffer->Width; x++) {
            u8 red = 0, green = 0, blue = 0;

            f32 dist = sqrtf(powf(f32(pos.x - x), 2) + powf(f32(pos.y / 2 - y), 2));

            red   = dist < size ? color[0] : 0;
            green = dist < size ? color[1] : 0;
            blue  = dist < size ? color[2] : 0;

            // Pixel: BB GG RR -- (4 bytes)
            //        0  1  2  3
            *pixel++ = u32(blue | (green << 8) | (red << 16));
        }

        row += stride;
    }
}

internal void Render(ScreenBuffer *buffer, const GameState *state) {
    u8 color[4] = {255, 0, 0, 255};
    DrawCircle(buffer, state->pos[0], 50, color);
}

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

    Render(screen, state);
}