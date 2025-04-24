#pragma once

#include "common.h"

typedef enum { ONESHOT, LOOPING, HELD } PlaybackType;

typedef struct SoundBuffer SoundBuffer;
typedef struct Sound       Sound;

Sound LoadSound(cstr path, PlaybackType type);
void  PlaySound(Sound sound);
void  PauseSound(Sound sound);
void  StopSound(Sound sound);
void  ResumeSound(Sound sound);