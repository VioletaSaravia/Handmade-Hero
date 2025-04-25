#pragma once
#include "common.h"

#include "audio.h"
#include "graphics.h"
#include "input.h"

// ===== MATH =====

f32 Rand() {
    return (f32)rand() / INT16_MAX;
}

#define MAX(a, b) (a >= b ? a : b)
#define MIN(a, b) (a <= b ? a : b)

inline v2 Normalize(v2 val) {
    f32 len = sqrtf(val.x * val.x + val.y * val.y);
    if (len > 0.0f) {
        val.x /= len;
        val.y /= len;
    }
    return val;
}

inline v2 Scale(v2 vec, f32 s) {
    return (v2){
        .x = vec.x * s,
        .y = vec.y * s,
    };
}

inline f32 Distance(v2 a, v2 b) {
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    return (sqrtf(dx * dx + dy * dy));
}

inline v2 Direction(v2 a, v2 b) {
    return Normalize((v2){b.x - a.x, b.y - a.y});
}

inline f32 Angle(v2 vec) {
    return atan2f(vec.y, vec.x);
}

inline f32 Length(v2 vec) {
    return sqrtf(vec.x * vec.x + vec.y * vec.y);
}

v2 CollisionNormal(Rect rectA, Rect rectB) {
    float dx = (rectA.x + rectA.w / 2) - (rectB.x + rectB.w / 2);
    float dy = (rectA.y + rectA.h / 2) - (rectB.y + rectB.h / 2);
    float px = (rectA.w + rectB.w) / 2 - fabsf(dx);
    float py = (rectA.h + rectB.h) / 2 - fabsf(dy);

    v2 normal = {0, 0};

    if (px < py) {
        // Collision is horizontal
        if (dx > 0)
            normal.x = 1; // From left
        else
            normal.x = -1; // From right
    } else {
        // Collision is vertical
        if (dy > 0)
            normal.y = 1; // From top
        else
            normal.y = -1; // From bottom
    }

    return normal;
}

inline v2 MoveBy(v2 a, v2 b, f32 amount) {
    v2  to   = (v2){b.x - a.x, b.y - a.y};
    f32 dist = Distance(a, b);

    if (dist == 0.0f) return (v2){0};

    return dist < amount ? to : Scale(Scale(to, 1.0f / dist), amount);
}

inline f32 f32Abs(f32 x) {
    return x < 0.0f ? -x : x;
}

#define EPSILON 0.01f
inline bool IsEq(f32 a, f32 b) {
    return f32Abs(a - b) < EPSILON;
}

inline bool IsEqV2(v2 a, v2 b) {
    return IsEq(a.x, b.x) && IsEq(a.y, b.y);
}

inline f32 CrossV2(v2 a, v2 b, v2 c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

inline f32 DistV2(v2 a, v2 b) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    return dx * dx + dy * dy;
}

typedef struct {
    v2 p0, p1, p2, p3;
} Bezier;

v2 bezierPoint(Bezier b, float t) {
    float u   = 1.0f - t;
    float tt  = t * t;
    float uu  = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    v2 p = {0};
    p.x  = uuu * b.p0.x + 3 * uu * t * b.p1.x + 3 * u * tt * b.p2.x + ttt * b.p3.x;

    p.y = uuu * b.p0.y + 3 * uu * t * b.p1.y + 3 * u * tt * b.p2.y + ttt * b.p3.y;

    return p;
}

// ===== ALGORITHMS =====

typedef enum ComponentType ComponentType;

#define MAX_COLLISIONS 4
typedef struct {
    cstr          key;
    ComponentType type;
    void         *data;
} Component;

typedef struct {
    u32 (*Hash)(const cstr);
    Component (*data)[MAX_COLLISIONS];
    u32 size;
    u32 entLen;
    u32 entSize;
} ComponentTable;
ComponentTable NewComponentTable(u32 size, u32 entSize);
void          *CompUpsert(ComponentTable *dict, const cstr key, void *data);
void          *CompInsert(ComponentTable *dict, const cstr key, void *data);
void          *CompUpdate(ComponentTable *dict, const cstr key, void *data);
void          *CompGet(const ComponentTable *dict, const cstr key);

u32 SimpleHash(cstr str) {
    u32 hash = 216;
    // for (u32 i = 0; i < 4 && *str; i++) {
    while (*str) {
        hash ^= (char)(*str++);
        hash *= 167;
    }
    return hash;
}

typedef struct {
    f32 _k1, _k2, _k3;
    f32 freq, damp, resp;
} Damper;

void DamperSetK(Damper *val) {
    val->_k1 = val->damp / (PI * val->freq);
    val->_k2 = 1 / ((TAU * val->freq) * (TAU * val->freq));
    val->_k3 = val->resp * val->damp / (TAU * val->freq);
}

void DamperSet(Damper *val, f32 f, f32 z, f32 r) {
    val->freq = f;
    val->damp = z;
    val->resp = r;

    DamperSetK(val);
}

Damper NewDamper(f32 f, f32 z, f32 r) {
    Damper result = (Damper){
        .freq = f,
        .damp = z,
        .resp = r,
    };
    DamperSetK(&result);
    return result;
}

typedef struct {
    f32  y, _xp, _yd;
    bool started, enabled;
} f32d;

f32d Newf32d(f32 x0) {
    return (f32d){
        .y       = x0,
        ._xp     = x0,
        .started = true,
        .enabled = true,
    };
}

void f32dUpdate(f32d *val, const Damper *damper, f32 delta, f32 x) {
    // if (!val->started) {}

    if (!val->enabled) {
        val->y = x;
        return;
    }

    f32 xd   = (x - val->_xp) / delta;
    val->_xp = x;
    val->y   = val->y + delta * val->_yd;

    f32 k2Stable = MAX(damper->_k2, 1.1f * (delta * delta / 4 + delta * damper->_k1 / 2));
    val->_yd =
        val->_yd + delta * (x + damper->_k3 * xd - val->y - damper->_k1 * val->_yd) / k2Stable;
}

Rect BoundingBoxOfSelection(const v2 *vects, const u32 count, const b64 selMap) {
    f32 minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX;
    for (u64 i = 0; i < count; i++) {
        if (!(selMap & (1ull << i))) continue;

        v2 vec = vects[i];
        if (vec.x < minX) minX = vec.x;
        if (vec.y < minY) minY = vec.y;
        if (vec.x > maxX) maxX = vec.x;
        if (vec.y > maxY) maxY = vec.y;
    }
    return (Rect){minX, minY, maxX - minX, maxY - minY};
}

global v2 pivot;
i32       ConvexHullCmp(const void *a, const void *b) {
    v2  p = *(v2 *)a;
    v2  q = *(v2 *)b;
    f32 c = CrossV2(pivot, p, q);
    if (c < 0) return 1;
    if (c > 0) return -1;
    // If collinear, nearest first
    return (DistV2(pivot, p) < DistV2(pivot, q)) ? -1 : 1;
}

// TODO No sorting in place
Poly ConvexHullOfSelection(v2 *points, u32 count, b32 bitmap);

// ===== IMAGE =====

#define WHITE (v4){1, 1, 1, 1}
#define BLACK (v4){0, 0, 0, 1}

typedef struct {
    u8 b, g, r, a;
} BMPColor;

typedef enum {
    RGB,
    RLE8,
    RLE4,
    BITFIELDS,
    JPEG,
    PNG,
    ALPHABITFIELDS,
    CMYK,
    CMYKRLE8,
    CMYKRLE4,
} BMPCompression;

typedef struct {
    // BMFH
    u16 bf_type;
    u32 bf_size;
    u16 bf_reserved1;
    u16 bf_reserved2;
    u32 bf_off_bits;

    // BMIH
    u32            size;
    i32            width;
    i32            height;
    u16            planes;
    u16            bit_count;
    BMPCompression compression;
    u32            size_image;
    v2i            pels_per_meter;
    u32            clr_used;
    u32            clr_important;

    // DATA
    BMPColor rgbq[256];
    u8       line_data[1024];
} AseBMP32x32;

typedef struct {
    u8 *data;
    u64 size;
} Image;

Image LoadBMP32x32Image(cstr path);

// ===== ENGINE =====

typedef struct GameSettings GameSettings;
GameSettings               *Settings();

typedef struct InputCtx InputCtx;
InputCtx               *Input();

typedef struct AudioCtx AudioCtx;
internal void           InitAudio(AudioCtx *ctx);
internal void           ShutdownAudio(AudioCtx *audio);
AudioCtx               *Audio();

typedef struct WindowCtx WindowCtx;
WindowCtx               *Window();
internal WindowCtx       InitWindow();
void                     ToggleFullscreen();

typedef struct GraphicsCtx GraphicsCtx;
GraphicsCtx               *Graphics();
GraphicsCtx                InitGraphics(WindowCtx *ctx, const GameSettings *settings);

typedef struct TimingCtx TimingCtx;
internal TimingCtx       InitTiming(f32 refreshRate);

typedef struct EngineCtx EngineCtx;
typedef struct GameState GameState;

typedef struct {
    void (*Setup)();
    void (*Init)();
    void (*Update)();
    void (*Draw)();
} GameCode;

// TODO Windows specific export keyword
__declspec(dllexport) void  EngineLoadGame(void (*setup)(), void (*init)(), void (*update)(),
                                           void (*draw)());
__declspec(dllexport) void  EngineReloadMemory(void *memory);
__declspec(dllexport) void  EngineInit();
__declspec(dllexport) void  EngineUpdate();
__declspec(dllexport) void  EngineShutdown();
__declspec(dllexport) void *EngineGetMemory();
__declspec(dllexport) bool  EngineIsRunning();

f32 Delta();
i32 Time();

// ===== MEMORY =====

#define ALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)

#if defined(_MSC_VER)
#include <intrin.h>
#define PopCnt64(x) __popcnt64(x)
#elif defined(__EMSCRIPTEN__) || defined(__GNUC__) || defined(__clang__)
#define PopCnt64(x) __builtin_popcountll(x)
#else
#error "Unsupported compiler"
#endif

// ===== FILES =====

__declspec(dllexport) u64 GetLastWriteTime(cstr file);
string                    ReadEntireFile(const char *filename);

// TODO
typedef struct {
    u8  *data;
    u64 *map;
    u64  count;
} DataPak;
DataPak NewDataPak(cstr folder);
DataPak LoadDataPak(cstr pakFile);
string  DataPakRead(cstr name);

// ===== GUI =====

typedef enum {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER_CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
} Anchor;

typedef enum { GUI_RELEASED, GUI_JUSTRELEASED, GUI_HOVERED, GUI_PRESSED, GUI_JUSTPRESSED } GuiState;
GuiState GuiButton(Rect button, cstr text);
GuiState GuiSlider(f32 *val, Rect coords, f32 from, f32 to);

typedef struct TextBox TextBox;
TextBox                NewTextBox(v2 size);
void                   DrawTextBox(TextBox *box);

// ===== DEBUG =====

typedef struct Logger Logger;

typedef enum { LEVEL_INFO, LEVEL_WARNING, LEVEL_ERROR, LEVEL_FATAL } LogLevel;

inline void Log(LogLevel level, cstr ctx, cstr msg) {
    cstr logLevel = level == LEVEL_FATAL     ? "Fatal"
                    : level == LEVEL_ERROR   ? "Error"
                    : level == LEVEL_WARNING ? "Warning"
                                             : "Info";

    printf("[%s] [%s] [%s] %s\n", __TIMESTAMP__, logLevel, ctx, msg);
}

#if LOG_LEVEL <= 0
#define LOG_INFO(msg) Log(LEVEL_INFO, __func__, msg)
#else
#define LOG_INFO(msg)
#endif

#if LOG_LEVEL <= 1
#define LOG_WARNING(msg) Log(LEVEL_WARNING, __func__, msg)
#else
#define LOG_WARNING(msg)
#endif

#if LOG_LEVEL <= 2
#define LOG_ERROR(msg) Log(LEVEL_ERROR, __func__, msg)
#else
#define LOG_ERROR(msg)
#endif

#if LOG_LEVEL <= 3
#define LOG_FATAL(msg)                                                                             \
    {                                                                                              \
        Log(LEVEL_FATAL, __func__, msg);                                                           \
        abort();                                                                                   \
    };
#else
#define LOG_FATAL(msg)
#endif
