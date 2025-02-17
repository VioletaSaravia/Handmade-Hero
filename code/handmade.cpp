#include "shared.h"

struct SoundBuffer {
    u8 *sampleOut;
    u32 byteCount;
    u32 sampleCount;
    u32 samplesPerSec;
};

internal void OutputSound(SoundBuffer *buffer) {
    f64 phase = 0;
    for (u32 i = 0; i < buffer->byteCount;) {
        i16 sample             = 0;
        buffer->sampleOut[i++] = (u8)sample;  // Values are little-endian.
        buffer->sampleOut[i++] = (u8)(sample >> 8);
    }
}

struct ScreenBuffer {
    void *Memory;
    int   Width, Height;
    int   BytesPerPixel;
};

internal void RenderWeirdGradient(ScreenBuffer *buffer, int xOffset, int yOffset) {
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

internal void UpdateAndRender(ScreenBuffer *screenBuffer, SoundBuffer *soundBuffer) {
    OutputSound(soundBuffer);
    RenderWeirdGradient(screenBuffer, 0, 0);
}