#include "audio.h"

void AudioStreamCallback(void *userData, SDL_AudioStream *stream, i32 additionalAmount,
                         i32 totalAmount) {
    SoundBuffer *sounds = (SoundBuffer *)userData;

    i32 channels   = 2; // sounds[0].spec.channels;
    i32 frameSize  = sizeof(f32) * channels;
    i32 frameCount = SDL_min(additionalAmount / frameSize, 2048);
    f32 temp[4096] = {0};

    for (u32 i = 0; i < MAX_SOUNDS; i++) {
        SoundBuffer *s = &sounds[i];
        if (s->played >= s->len - frameSize && s->type == LOOPING) s->played = 0;
        if (s->played >= s->len - frameSize && s->type != LOOPING) s->playing = false;
        if (!s->playing) continue;

        i32 availableFrames = (s->len - s->played) / frameSize;
        i32 toWrite         = SDL_min(frameCount, availableFrames);

        f32 *src       = (f32 *)(s->data + s->played);
        f32  leftGain  = s->vol * (1.0f - s->pan) * 0.5f;
        f32  rightGain = s->vol * (1.0f + s->pan) * 0.5f;
        leftGain       = fmaxf(0.0f, leftGain);
        rightGain      = fmaxf(0.0f, rightGain);
        for (i32 j = 0; j < toWrite * channels; j++) {
            i32 id = j * channels;
            temp[id + 0] += src[id + 0] * 0.2;
            temp[id + 1] += src[id + 1] * rightGain;
        }

        s->played += toWrite * frameSize;
    }

    SDL_PutAudioStreamData(stream, temp, frameCount * frameSize);
}

void InitAudio(AudioCtx *ctx) {
    ctx->deviceId      = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, 0);
    SDL_AudioSpec spec = {
        .channels = 2,
        .format   = SDL_AUDIO_S32LE,
        .freq     = 48000,
    };
    ctx->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                            AudioStreamCallback, ctx->sounds);

    SDL_GetAudioStreamFormat(ctx->stream, &ctx->srcSpec, &ctx->dstSpec);
    SDL_ResumeAudioStreamDevice(ctx->stream);
    return;
}

void ShutdownAudio(AudioCtx *audio) {
    SDL_DestroyAudioStream(Audio()->stream);
}

Sound NewSound(cstr path, PlaybackType type) {
    SoundBuffer result = {
        .type = type,
        .vol  = 1.0f,
    };

    if (!SDL_LoadWAV(path, &result.spec, &result.data, &result.len)) {
        LOG_ERROR("Loading file %s failed: %s", "data\\test.wav", SDL_GetError());
        return (Sound){0};
    }

    result.audioStream = SDL_CreateAudioStream(&result.spec, 0);
    if (!result.audioStream) LOG_ERROR("Failed to create audio stream: %s", SDL_GetError());
    if (!SDL_BindAudioStream(Audio()->deviceId, result.audioStream))
        LOG_ERROR("Failed to bind audio stream: %s", SDL_GetError());

    Audio()->sounds[Audio()->count] = result;

    return (Sound){.id = Audio()->count++};
}

void SoundPlay(Sound sound) {
    SoundBuffer *buf = &Audio()->sounds[sound.id];
    SDL_ClearAudioStream(buf->audioStream);
    buf->playing = true;
    buf->played  = 0;
}

void SoundPause(Sound sound) {
    SoundBuffer *buf = &Audio()->sounds[sound.id];
    buf->playing     = false;
}

void SoundStop(Sound sound) {
    SoundBuffer *buf = &Audio()->sounds[sound.id];
    SDL_ClearAudioStream(buf->audioStream);
    buf->played = 0;
}

void SoundResume(Sound sound) {
    SoundBuffer *buf = &Audio()->sounds[sound.id];
    buf->playing     = true;
}

void SoundSetPan(Sound sound, f32 pan) {
    SoundBuffer *buf = &Audio()->sounds[sound.id];
    buf->pan         = pan;
}

void SoundSetVol(Sound sound, f32 vol) {
    SoundBuffer *buf = &Audio()->sounds[sound.id];
    buf->vol         = vol;
}