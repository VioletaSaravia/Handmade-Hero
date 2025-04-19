#include "win32.c"

/*  TODO
    - [X] Shift+move queue
    - [X] Cleanup day
    - [X] Sound
    - [X] Rect collision
    - [ ] Optimize collision
    - [ ] Global allocator

    FIXME
    - [X] Calculate mouse pos only once per frame
*/

#define MOVE_LIMIT 32
struct MoveList {
    v2               target;
    struct MoveList *next;
};
typedef struct MoveList MoveList;

typedef struct {
    u32      count;
    Texture *tex;
    f32     *speed;
} UnitTypes;

typedef struct {
    MemRegion  buffer;
    u32        count, max;
    Dictionary components;
} Entities;

typedef struct {
    Texture selector;
    bool    selecting;
    Rect    selBox;
    b64     selMap;
} SelectionCtx;

typedef struct {
    Camera cam;
    Sound  sounds[MAX_SOUNDS];

    SelectionCtx selCtx;
    Entities     units;
    UnitTypes    unitTypes;
} GameState;
GameState *S;

extern void Setup() {
    E->Settings = (GameSettings){
        .name       = (cstr)L"Test Game",
        .version    = "0.2",
        .resolution = (v2i){640, 360},
        .scale      = 1,
    };
}

extern void Init() {
    S->unitTypes = (UnitTypes){
        .count = 8,
        .tex   = ALLOC(sizeof(Texture) + S->unitTypes.count),
        .speed = ALLOC(sizeof(f32) + S->unitTypes.count),
    };
    S->unitTypes.tex[0]   = NewTexture("data\\ship.png");
    S->unitTypes.speed[0] = 100;
    S->sounds[0]          = LoadSound("data\\gun.wav", ONESHOT);

    S->selCtx.selector = NewTexture("data\\selector_square_32x32.png");
    S->cam             = (Camera){(v2){0}, 1.0f, 200};

    S->units = (Entities){.buffer     = NewMemRegion(64 * 10000),
                          .components = NewDictionary(64),
                          .count      = 64,
                          .max        = 64};

    Dictionary *comps     = &S->units.components;
    v2         *positions = (v2 *)DictInsert(comps, "pos", ALLOC(S->units.max * sizeof(v2)));
    MoveList   *targets =
        (MoveList *)DictInsert(comps, "targets", ALLOC(S->units.max * sizeof(MoveList)));
    v2  *colliders = (v2 *)DictInsert(comps, "colliders", ALLOC(S->units.max * sizeof(v2)));
    u32 *types     = (u32 *)DictInsert(comps, "types", ALLOC(S->units.max * sizeof(u32)));
    u32 *idx       = (u32 *)DictInsert(comps, "idx", ALLOC(S->units.max * sizeof(u32)));

    for (u64 i = 0; i < S->units.count; i++) {
        positions[i] = (v2){Rand() * 640, Rand() * 360};
        targets[i]   = (MoveList){positions[i], 0};
        types[i]     = 0;
        colliders[i] = (v2){
            (S->unitTypes.tex[types[i]].size).x,
            (S->unitTypes.tex[types[i]].size).y,
        };
    }
}

void ProcessWASDCamera(Camera *cam) {
    v2 move = {0, 0};
    if (Input()->keys['W'] >= Pressed) move.y -= 1;
    if (Input()->keys['A'] >= Pressed) move.x -= 1;
    if (Input()->keys['S'] >= Pressed) move.y += 1;
    if (Input()->keys['D'] >= Pressed) move.x += 1;
    move = Normalize(move);
    S->cam.pos.x += move.x * cam->speed * Delta();
    S->cam.pos.y += move.y * cam->speed * Delta();
}

void ProcessMouseSelection(SelectionCtx *ctx, const Entities *units, const UnitTypes *types) {
    const v2  *pos    = DictGet(&units->components, "pos");
    const u32 *uTypes = DictGet(&units->components, "types");

    switch (Input()->mouse.left) {
    case JustPressed:
        ctx->selecting   = true;
        ctx->selBox.pos  = MouseInWorld(S->cam);
        ctx->selBox.size = (v2){0};
        if (Input()->keys[KEY_Shift] != Pressed) {
            ctx->selMap = 0;
        }
        for (u64 i = 0; i < units->count; i++) {
            v2i size = types->tex[uTypes[i]].size;
            if (V2InRect(MouseInWorld(S->cam),
                         (Rect){(v2){pos[i].x - size.x / 2, pos[i].y - size.y / 2},
                                (v2){size.x, size.y}})) {
                ctx->selMap |= (1ull << i);
                break;
            }
        }
        break;

    case Pressed:
        v2 mouse         = MouseInWorld(S->cam);
        ctx->selBox.size = (v2){mouse.x - ctx->selBox.pos.x, mouse.y - ctx->selBox.pos.y};
        if (Input()->keys[KEY_Shift] != Pressed) {
            ctx->selMap = PopCnt64(ctx->selMap) == 1 ? ctx->selMap : 0;
        }
        for (u64 i = 0; i < units->count; i++) {
            if (V2InRect(pos[i], ctx->selBox)) {
                ctx->selMap |= (1ull << i);
            }
        }
        break;

    case JustReleased: {
        ctx->selecting = false;
        ctx->selBox    = (Rect){0};
        break;
    }
    }
}

void ProcessMovementToTarget(const SelectionCtx *ctx, Entities *units) {
    const v2 *pos     = DictGet(&units->components, "pos");
    MoveList *targets = DictGet(&units->components, "targets");

    if ((!ctx->selecting) && Input()->mouse.right == JustPressed) {
        PlaySound(S->sounds[0]);
        Rect box       = BoundingBoxOfSelection(pos, units->count, ctx->selMap);
        v2   boxCenter = (v2){box.x + box.w / 2, box.y + box.h / 2};
        for (u64 i = 0; i < units->count; i++) {
            if (!(ctx->selMap & (1ull << i))) continue;

            v2 iPos         = pos[i];
            v2 cursor       = MouseInWorld(S->cam);
            v2 centerOffset = (v2){boxCenter.x - iPos.x, boxCenter.y - iPos.y};

            v2 target = V2InRect(cursor, box)
                            ? cursor
                            : (v2){cursor.x - centerOffset.x, cursor.y - centerOffset.y};

            if (Input()->keys[KEY_Shift] == Pressed) {
                MoveList *last = &targets[i];
                for (u32 j = 0; j < MOVE_LIMIT; j++) {
                    if (last->next)
                        last = last->next;
                    else
                        break;
                }

                last->next  = (MoveList *)RingAlloc(&units->buffer, sizeof(MoveList));
                *last->next = (MoveList){target, 0};
            } else {
                targets[i] = (MoveList){target, 0};
            }
        }
    }
}

void CalculateMovementToTargetWithCollision(const Entities *units, const UnitTypes *types) {
    v2        *pos       = DictGet(&units->components, "pos");
    MoveList  *targets   = DictGet(&units->components, "targets");
    const u32 *uTypes    = DictGet(&units->components, "types");
    const v2  *colliders = DictGet(&units->components, "colliders");

    for (u32 i = 0; i < units->count; i++) {
        f32 speed = types->speed[uTypes[i]];

        bool arrived = IsEqV2(pos[i], targets[i].target);
        if (arrived && targets[i].next) targets[i] = *targets[i].next;
        if (arrived) continue;

        v2 move = MoveBy(pos[i], targets[i].target, speed * Delta());
        pos[i].x += move.x;
        pos[i].y += move.y;

        for (u32 j = 0; j < units->count; j++) {
            if (i == j) continue;
            Rect from = (Rect){pos[i], colliders[i]};
            Rect to   = (Rect){pos[j], colliders[j]};
            if (!CollisionRectRect(from, to)) continue;
            v2 normal = CollisionNormal(from, to);

            if (normal.x == 0) continue;
            if (normal.x > 0)
                pos[i].x = pos[j].x + colliders[j].w;
            else
                pos[i].x = pos[j].x - colliders[i].w;
        }

        for (u32 j = 0; j < units->count; j++) {
            if (i == j) continue;
            Rect from = (Rect){pos[i], colliders[i]};
            Rect to   = (Rect){pos[j], colliders[j]};
            if (!CollisionRectRect(from, to)) continue;
            v2 normal = CollisionNormal(from, to);

            if (normal.y == 0) continue;
            if (normal.y > 0)
                pos[i].y = pos[j].y + colliders[j].h;
            else
                pos[i].y = pos[j].y - colliders[i].h;
        }
    }
}

void DrawUnits(const UnitTypes *types, const Entities *units, const SelectionCtx *ctx) {
    const v2  *pos    = DictGet(&units->components, "pos");
    const u32 *uTypes = DictGet(&units->components, "types");

    for (u64 i = 0; i < units->count; i++) {
        Texture tex = types->tex[uTypes[i]];

        DrawTexture(tex, (v2){pos[i].x - tex.size.x / 2, pos[i].y - tex.size.y / 2}, 1);
        if (ctx->selMap & (1ull << i)) {
            DrawTexture(ctx->selector, (v2){pos[i].x - tex.size.x / 2, pos[i].y - tex.size.y / 2},
                        1);
        }
    }
}

extern void Update() {
    ProcessMouseSelection(&S->selCtx, &S->units, &S->unitTypes);
    ProcessMovementToTarget(&S->selCtx, &S->units);
    CalculateMovementToTargetWithCollision(&S->units, &S->unitTypes);

    ProcessWASDCamera(&S->cam);
}

extern void Draw() {
    CameraBegin(S->cam);
    DrawUnits(&S->unitTypes, &S->units, &S->selCtx);
    DrawRectangle(S->selCtx.selBox, (v4){0.2, 0.2, 0.6, 0.4}, 5);
    CameraEnd();

    f32 scale = 2;
    DrawRectangle((Rect){0, -10, 640, 14 * scale + 10}, (v4){0, 0, 0, 1}, 0);
    DrawText(" [File]  [Edit]  [View]  [Help]", (v2){0, 2 * scale}, 0, scale);
    GuiButton((Rect){50, 50, 100, 24}, "Click me!");
    DrawLine((v2){100, 100}, (v2){400, 300}, WHITE, 1);
}
