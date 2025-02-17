#include "shared.h"

struct GameOffscreenBuffer {
    void *Memory;
    int   Width, Height;
    int   BytesPerPixel;
};

internal void RenderWeirdGradient(GameOffscreenBuffer *buffer, int xOffset, int yOffset) {
    int width  = buffer->Width;
    int height = buffer->Height;

    int stride = width * buffer->BytesPerPixel;
    u8 *row    = (u8 *)buffer->Memory;
    for (int y = 0; y < height; y++) {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < width; x++) {
            /*
            Pixel: BB GG RR -- (4 bytes)
                   0  1  2  3
            */
            u8 blue  = u8(x + xOffset);
            u8 green = u8(y + yOffset);
            u8 red   = 0;

            *pixel++ = u32(blue | (green << 8) | (red << 16));
        }

        row += stride;
    }
}

internal void UpdateAndRender(GameOffscreenBuffer *buffer) { RenderWeirdGradient(buffer, 0, 0); }