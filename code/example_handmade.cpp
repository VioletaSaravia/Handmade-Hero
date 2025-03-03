#include "handmade.h"

void *PlatformReadEntireFile(const char *filename) { return 0; }

bool PlatformWriteEntireFile(char *filename, u32 size, void *memory) { return false; }

void PlatformFreeFileMemory(void *memory) {}

struct ExampleMemoryArena : Memory {};
global ExampleMemoryArena ExampleMemory;

struct ExampleScreenBuffer : ScreenBuffer {};
global ExampleScreenBuffer ExampleScreen;

internal bool ExampleInitWindow(ExampleScreenBuffer *screen) { return false; }

struct ExampleSoundBuffer : SoundBuffer {};
global ExampleSoundBuffer ExampleSound;

internal void ExampleInitSound(ExampleSoundBuffer *sound) {}

struct ExampleInputBuffer : InputBuffer {};
global ExampleInputBuffer ExampleInput;

internal void ExampleProcessInput(ExampleInputBuffer *state) {}

struct ExampleGameCode {
    GameInit   *Init;
    GameUpdate *Update;
};
global ExampleGameCode Game;

internal ExampleGameCode ExampleLoadGame() { return {}; }

internal void ExampleUpdate() {
    Game.Update(&ExampleMemory, &ExampleInput, &ExampleScreen, &ExampleSound);
}

global bool Running = true;

i32 main() {
    ExampleInitWindow(&ExampleScreen);
    ExampleInitSound(&ExampleSound);

    ExampleMemory = {};
    Game.Init(&ExampleMemory);

    while (Running) {
        ExampleProcessInput(&ExampleInput);
    }
}