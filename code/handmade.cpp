#include "handmade.h"

internal void OutputSound(const GameState *state, SoundBuffer *buffer) {}

internal void Render(ScreenBuffer *buffer, const GameState *state) {
    i32 stride = buffer->Width * buffer->BytesPerPixel;
    u8 *row    = (u8 *)buffer->Memory;
    for (i32 y = 0; y < buffer->Height; y++) {
        u32 *pixel = (u32 *)row;
        for (i32 x = 0; x < buffer->Width; x++) {
            u8 red = 0, green = 0, blue = 0;

            for (i32 i = 0; i < 1; i++) {
                f32 dist = sqrtf(powf(f32(state->pos[i].x - x), 2) +
                                 powf(f32(state->pos[i].y / 2 - y), 2));

                red += dist < 50 ? 255 : 0;
            }

            // Pixel: BB GG RR -- (4 bytes)
            //        0  1  2  3
            *pixel++ = u32(blue | (green << 8) | (red << 16));
        }

        row += stride;
    }
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
        state->pos[0].x += 10;
    }
    if (input->keyboard.keys['A'] >= ::Pressed) {
        state->pos[0].x -= 10;
    }
    if (input->keyboard.keys['W'] >= ::Pressed) {
        state->pos[0].y -= 10;
    }
    if (input->keyboard.keys['S'] >= ::Pressed) {
        state->pos[0].y += 10;
    }

    OutputSound(state, sound);
    Render(screen, state);
}