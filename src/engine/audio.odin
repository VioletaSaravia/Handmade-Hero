package engine

import ma "vendor:miniaudio"

AudioBuffer :: struct {
	device: ma.device,
}

AudioDataCallback :: proc "c" (
	device: ^ma.device,
	output: rawptr,
	input: rawptr,
	frame_count: u32,
) {
	out := ([^]f32)(output)
	for f: u32; f < frame_count; f += 2 {
		out[f] = 0
		out[f + 1] = 0
	}
}

InitAudio :: proc() -> (result: AudioBuffer, ok: bool = false) {
	config := ma.device_config_init(.playback)
	config.playback.format = .f32 // PCM format
	config.playback.channels = 0 // Use device's native
	config.playback.pDeviceID = nil

	config.sampleRate = 0 // Use device's native
	config.dataCallback = AudioDataCallback
	config.pUserData = nil // ???

	ma.device_init(nil, &config, &result.device)
	ma.device_start(&result.device)
	return
}

StopAudio :: proc(audio: ^AudioBuffer) {
	ma.device_stop(&audio.device)
	ma.device_uninit(&audio.device)
}

Sound :: struct {
	data:    []byte,
	volume:  f32,
	playing: bool,
}

LoadSound :: proc(path: string) -> Sound {
	return {}
}
