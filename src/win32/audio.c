#include "win32.h"

void AudioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    SoundBuffer *sounds = (SoundBuffer *)pDevice->pUserData;
    f32         *out    = (f32 *)pOutput;
    memset(out, 0, frameCount * sizeof(f32) * 2);

    f32 temp[4096] = {0};

    for (u32 i = 0; i < MAX_SOUNDS; i++) {
        SoundBuffer *s = &sounds[i];
        if (!s->playing) continue;

        u64 read = 0;
        ma_decoder_read_pcm_frames(&s->decoder, temp, frameCount, &read);

        if (read == 0) {
            s->playing = (s->type == LOOPING);
            ma_decoder_seek_to_pcm_frame(&s->decoder, 0);
            continue;
        }

        f32 leftGain  = 1.0f - (s->pan > 0 ? s->pan : 0.0f);
        f32 rightGain = 1.0f + (s->pan < 0 ? s->pan : 0.0f);
        for (size_t j = 0; j < read * 2 /* TODO CHANNELS */; j++) {
            out[j] /* CHANNELS */ += temp[j] * s->vol * (j % 2 == 0 ? leftGain : rightGain);
        }
    }
}

void InitAudio(AudioCtx *ctx) {
    ctx->config = ma_device_config_init(ma_device_type_playback);

    ctx->config.playback.format   = ma_format_f32; // Set to ma_format_unknown to use the device's
    ctx->config.playback.channels = 2;     // Set to 0 to use the device's native channel count.
    ctx->config.sampleRate        = 48000; // Set to 0 to use the device's native sample rate.
    ctx->config.dataCallback      = AudioCallback; // called when miniaudio needs more data.
    ctx->config.pUserData         = ctx->sounds;   // Can be accessed from (device.pUserData).

    if (ma_device_init(0, &ctx->config, &ctx->device) != MA_SUCCESS)
        LOG_FATAL("Couldn't initialize audio device")

    ma_result err = ma_device_start(&ctx->device);
    if (err != MA_SUCCESS) LOG_FATAL("Couldn't start audio device")

    return;
}

void ShutdownAudio(AudioCtx *audio) {
    ma_device_stop(&audio->device);
    ma_device_uninit(&audio->device);
    for (size_t i = 0; i < MAX_SOUNDS; i++) {
        ma_decoder_uninit(&Audio()->sounds[i].decoder);
    }
}

Sound LoadSound(cstr path, PlaybackType type) {
    string      data   = ReadEntireFile(path);
    SoundBuffer result = {
        .data    = (u16 *)data.data,
        .dataLen = data.len,
        .vol     = 1.0,
        .pan     = 0.0,
        .type    = type,
    };

    ma_decoder_config config =
        ma_decoder_config_init(Audio()->device.playback.format, Audio()->device.playback.channels,
                               Audio()->device.sampleRate);

    i32 err = ma_decoder_init_memory(result.data, result.dataLen, &config, &result.decoder);
    if (err != MA_SUCCESS) {
        printf("[Error] [%s] Can't initialize memory: %d", __func__, err);
        return (Sound){-1};
    }

    u32 id              = Audio()->count;
    Audio()->sounds[id] = result;
    Audio()->count      = (id + 1) % MAX_SOUNDS;
    return (Sound){id};
}

void PlaySound(Sound sound) {
    SoundBuffer *buffer = &Audio()->sounds[sound.id];
    buffer->playing     = true;
    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
}

void PauseSound(Sound sound) {
    Audio()->sounds[sound.id].playing = false;
}

void StopSound(Sound sound) {
    SoundBuffer *buffer = &Audio()->sounds[sound.id];
    buffer->playing     = false;
    ma_decoder_seek_to_pcm_frame(&buffer->decoder, 0);
}

void ResumeSound(Sound sound) {
    SoundBuffer *buffer = &Audio()->sounds[sound.id];
    buffer->playing     = true;
}