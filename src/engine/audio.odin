package engine

import "core:fmt"
import ma "vendor:miniaudio"

AUDIO_CHANNELS :: 2
AUDIO_SAMPLE_RATE :: 44100
AUDIO_FORMAT :: ma.format.f32

MAX_SOUNDS :: 16

AudioBuffer :: struct {
	device: ma.device,
	sounds: [MAX_SOUNDS]Sound,
}

AudioDataCallback :: proc "c" (
	device: ^ma.device,
	output: rawptr,
	input: rawptr,
	frame_count: u32,
) {
	if device.pUserData == nil do return
	out := ([^]f32)(output)
	sounds := (^[8]Sound)(device.pUserData)

	temp: [4096]f32

	for &s in sounds {
		if !s.playing do continue

		read: u64
		ma.decoder_read_pcm_frames(&s.decoder, &temp, auto_cast frame_count, &read)
		if read == 0 {
			// FIXME(viole): Se saltea un callback entero de frame como máximo.
			s.playing = s.type != .looping ? false : true
			ma.decoder_seek_to_pcm_frame(&s.decoder, 0)
		}

		for i in 0 ..< read * AUDIO_CHANNELS {
			pan := i % 2 == 0 ? s.pan * -1 : s.pan
			if pan > 0 do pan = 0
			if pan < -1 do pan = -1

			out[(cast(u64)frame_count - read) * AUDIO_CHANNELS + i] += (temp[i] * (s.volume + pan))
		}
	}
}

InitAudio :: proc(buffer: ^AudioBuffer) -> bool {
	config := ma.device_config_init(.playback)
	config.playback.format = AUDIO_FORMAT
	config.playback.channels = AUDIO_CHANNELS

	config.sampleRate = AUDIO_SAMPLE_RATE
	config.dataCallback = AudioDataCallback
	config.pUserData = &buffer.sounds

	if err := ma.device_init(nil, &config, &buffer.device); err != ma.result.SUCCESS {
		fmt.println(err)
		return false
	}
	if err := ma.device_start(&buffer.device); err != ma.result.SUCCESS {
		fmt.println(err)
		ma.device_uninit(&buffer.device)
		return false
	}

	return true
}

ShutdownAudio :: proc(audio: ^AudioBuffer) {
	ma.device_stop(&audio.device)
	ma.device_uninit(&audio.device)
	for &s in audio.sounds do _ = ma.decoder_uninit(&s.decoder)
}

PlaybackType :: enum {
	oneshot,
	looping,
	held,
}

SoundType :: union {
	ma.waveform,
	ma.decoder,
}

Sound :: struct {
	data:    []byte,
	volume:  f32, // [0; 1]
	pan:     f32, // [-1; 1]
	type:    PlaybackType,
	playing: bool,
	decoder: ma.decoder,
}

LoadSound :: proc($path: string, type: PlaybackType = .oneshot) -> (result: Sound) {
	result = {
		data   = #load(DATA + path, []byte),
		volume = 1.0,
		type   = type,
	}

	// No hace falta, pero es más eficiente que dejar que miniaudio adivine el formato.
	config := ma.decoder_config {
		format     = AUDIO_FORMAT,
		channels   = AUDIO_CHANNELS,
		sampleRate = AUDIO_SAMPLE_RATE,
		// encodingFormat = .wav,
	}

	if err := ma.decoder_init_memory(
		raw_data(result.data),
		len(result.data),
		&config,
		&result.decoder,
	); err != ma.result.SUCCESS {
		fmt.println(err)
	}
	return
}

PlaySound :: proc(sound: ^Sound) {
	sound.playing = true
	ma.decoder_seek_to_pcm_frame(&sound.decoder, 0)
}

StopSound :: proc(sound: ^Sound) {
	sound.playing = false
	ma.decoder_seek_to_pcm_frame(&sound.decoder, 0)
}

PauseSound :: proc(sound: ^Sound) {
	sound.playing = false
}

ResumeSound :: proc(sound: ^Sound) {
	sound.playing = true
}
