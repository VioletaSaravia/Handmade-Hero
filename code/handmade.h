#pragma once

#include "shared.h"

#ifndef HANDMADE_INTERNAL  // TODO invert
void* PlatformReadEntireFile(char* filename);
void  PlatformFreeFileMemory(void* memory);
bool  PlatformWriteEntireFile(char* filename, u32 size, void* memory);
#endif