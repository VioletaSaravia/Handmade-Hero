#pragma once

#include "engine.h"
#include "graphics.h"
#include "input.h"

bool GuiButton(Rect rect);
bool GuiToggle(Rect rect, bool *val);
bool GuiSliderV(v2 pos, f32 size, f32 *val);
bool GuiSliderH(v2 pos, f32 size, f32 *val);