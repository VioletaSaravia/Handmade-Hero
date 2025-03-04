#pragma once

#include <strsafe.h>  // StringCbPrintfA()
#include <Windows.h>
#include <xaudio2.h>
#include <Xinput.h>

#include "glad/glad.h"
#include "handmade.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// <Debug>
#ifndef DEBUG
#define WIN32_CHECK(func) (func)
#else
#define WIN32_CHECK(func)                                                        \
    if (FAILED(func)) {                                                          \
        char buf[32];                                                            \
        StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %d\n", GetLastError()); \
        OutputDebugStringA(buf);                                                 \
    }
#endif

#ifndef DEBUG
#define WIN32_LOG(str)
#else
#define WIN32_LOG(str)                                            \
    char buf[128];                                                \
    StringCbPrintfA(buf, sizeof(buf), "[WIN32 ERROR] %s\n", str); \
    OutputDebugStringA(buf);
#endif

global Object Objects[8];
global u32    ObjCount;

struct Win32GameCode {
    HMODULE     dll;
    GameInit   *Init;
    GameUpdate *Update;
};

global bool Running = true;

struct Win32ScreenBuffer : ScreenBuffer {
    HWND       window;
    HDC        deviceContext;
    DWORD      refreshRate;
    i64        perfCountFrequency;
    BITMAPINFO Info;
};
global Win32ScreenBuffer Win32Screen;

struct Win32InputBuffer : InputBuffer {};
global Win32InputBuffer Win32Input;

global Memory Win32Memory;

struct Win32SoundBuffer : SoundBuffer {
    IXAudio2               *xAudio2;
    IXAudio2MasteringVoice *xAudio2MasteringVoice;
    IXAudio2SourceVoice    *xAudio2TestSourceVoice;
};
global Win32SoundBuffer Win32Sound;

struct Win32TimingBuffer {
    // Used by Sleep()
    u32  desiredSchedulerMs;
    bool granularSleepOn;

    f64 targetSPF;
    i64 lastCounter;
    u64 lastCycleCount;
    f64 delta;
};
global Win32TimingBuffer Win32Timing;

struct SoundTone {
    u8 *buf;
    u32 byteCount;
    u32 sampleCount;

    u32 cyclesPerSec;
    f32 samplesPerCycle;
    u16 bufferSizeInCycles;

    IXAudio2SourceVoice *xAudio2Voice;
};