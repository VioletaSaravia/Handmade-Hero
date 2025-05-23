#pragma once

#include "engine.h"

typedef enum { ONESHOT, LOOPING, HELD } PlaybackType;

typedef struct SoundBuffer {
    SDL_AudioStream *audioStream;
    SDL_AudioSpec    spec;
    u8              *data;
    u32              len, played;
    f32              vol, pan;
    PlaybackType     type;
    bool             playing;
} SoundBuffer;

typedef struct Sound {
    u32 id;
} Sound;

Sound NewSound(cstr path, PlaybackType type);
void  SoundPlay(Sound sound);
void  SoundPause(Sound sound);
void  SoundStop(Sound sound);
void  SoundResume(Sound sound);
void  SoundSetPan(Sound sound, f32 pan);
void  SoundSetVol(Sound sound, f32 vol);

typedef struct {
    SDL_AudioDeviceID deviceId;
    SDL_AudioStream  *stream;
    SDL_AudioSpec     srcSpec, dstSpec;
    SoundBuffer      *sounds;
    u32               soundsMax, soundsCount;
} AudioCtx;
AudioCtx  InitAudio();
void      ShutdownAudio(AudioCtx *audio);
AudioCtx *Audio();