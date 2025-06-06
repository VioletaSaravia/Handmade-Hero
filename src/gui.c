#include "gui.h"

#define BORDERS 0.1
#define THICKNESS 2

bool GuiButton(Rect rect) {
    bool justPressed = GetMouseButton(BUTTON_LEFT) == JustPressed;
    bool pressed     = GetMouseButton(BUTTON_LEFT) >= Pressed;
    bool inRect      = V2InRect(Mouse(), rect);

    DrawRectangle(rect, 0, pressed && inRect ? WHITE : BLACK, BORDERS);
    return justPressed && inRect;
}

bool GuiToggle(Rect rect, bool *val) {
    bool pressed = GetMouseButton(BUTTON_LEFT) == JustPressed;
    bool inRect  = V2InRect(Mouse(), rect);

    if (pressed && inRect) *val ^= true;

    DrawRectangle(rect, 0, *val ? WHITE : BLACK, BORDERS);
    return pressed && inRect;
}

bool GuiSliderV(v2 pos, f32 size, f32 *val) {
    bool pressed = GetMouseButton(BUTTON_LEFT) >= Pressed;
    v2   mouse   = Mouse();

    persist f32 width = 10;

    Rect slider = (Rect){pos.x, pos.y, width, size};
    bool inRect = V2InRect(mouse, slider);

    if (pressed && inRect) *val = 1 - ((mouse.y - pos.y) / size);

    f32 btnRadius = size * (1 - *val);
    DrawRectangle(slider, 0, BLACK, BORDERS);
    DrawRectangle((Rect){pos.x, pos.y + btnRadius - width / 2, width, width}, 0, WHITE, BORDERS);

    return pressed && inRect;
}

bool GuiSliderH(v2 pos, f32 size, f32 *val) {
    persist f32 width   = 10;
    bool        pressed = GetMouseButton(BUTTON_LEFT) >= Pressed;
    v2          mouse   = Mouse();

    Rect slider = (Rect){pos.x, pos.y, size, width};
    bool inRect = V2InRect(mouse, slider);

    if (pressed && inRect) *val = ((mouse.x - pos.x) / size);

    f32 btnRadius = size * (*val);
    DrawRectangle(slider, 0, BLACK, BORDERS);
    DrawRectangle((Rect){pos.x + btnRadius - width / 2, pos.y, width, width}, 0, WHITE, BORDERS);

    return pressed && inRect;
}

bool GuiCounter(v2 pos, f32 size, i32 *val) {
    bool leftJustPressed = GuiButton((Rect){pos.x, pos.y, size, size});
    DrawRectangle((Rect){pos.x + size * 1.2, pos.y, size * 2, size}, 0, WHITE, BORDERS);
    bool rightJustPressed = GuiButton((Rect){pos.x + size * 3.4, pos.y, size, size});

    if (leftJustPressed) *val -= 1;
    if (rightJustPressed) *val += 1;

    return leftJustPressed || rightJustPressed;
}