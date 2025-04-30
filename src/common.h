#pragma once

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <glad.h>

#ifdef _WIN32
#define export __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define export __attribute__((visibility("default")))
#else
#define export
#endif

#define intern static
#define global static
#define persist static

typedef float_t  f32;
typedef double_t f64;

#define PI32 3.14159265f
#define PI64 3.141592653589793
#define PI PI32
#define TAU32 6.2831853f
#define TAU64 6.283185307179586
#define TAU TAU32

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef char *cstr;

typedef struct {
    cstr data;
    u64  len;
} Array;

typedef Array string;
string        String(cstr data);

typedef union {
    struct {
        f32 x, y;
    };
    struct {
        f32 w, h;
    };
} v2;

typedef struct {
    bool x, y;
} v2b;

inline v2 v2Add(v2 a, v2 b) {
    return (v2){a.x + b.x, a.y + b.y};
}

inline v2 v2Sub(v2 a, v2 b) {
    return (v2){a.x - b.x, a.y - b.y};
}

inline v2 v2Scale(v2 a, f32 s) {
    return (v2){a.x * s, a.y * s};
}

typedef union {
    struct {
        i32 x, y;
    };
    struct {
        i32 w, h;
    };
} v2i;

typedef union {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
} v3;

typedef union {
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
} v4;

typedef union {
    struct {
        v2 pos, size;
    };
    struct {
        f32 x, y, w, h;
    };
} Rect;
bool V2InRect(v2 pos, Rect rectangle);

typedef struct {
    v2  pos;
    f32 radius;
} Circle;

typedef struct {
    v2 *verts;
    u32 count;
} Poly;

typedef struct {
    u8 *buf;
    u64 used, size;
} Arena;
Arena NewArena(void *memory, u64 size);
void *Alloc(Arena *arena, u64 size);
void *RingAlloc(Arena *arena, u64 size);
void  DeAlloc(Arena *arena, void *ptr);
void  Empty(Arena *arena);

// ===== MATH =====

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

v2 CollisionNormal(Rect rectA, Rect rectB);

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

typedef struct {
    f32 _k1, _k2, _k3;
    f32 freq, damp, resp;
} Damper;

inline void DamperSetK(Damper *val) {
    val->_k1 = val->damp / (PI * val->freq);
    val->_k2 = 1 / ((TAU * val->freq) * (TAU * val->freq));
    val->_k3 = val->resp * val->damp / (TAU * val->freq);
}

inline void DamperSet(Damper *val, f32 f, f32 z, f32 r) {
    val->freq = f;
    val->damp = z;
    val->resp = r;

    DamperSetK(val);
}

inline Damper NewDamper(f32 f, f32 z, f32 r) {
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

inline f32d Newf32d(f32 x0) {
    return (f32d){
        .y       = x0,
        ._xp     = x0,
        .started = true,
        .enabled = true,
    };
}

inline void f32dUpdate(f32d *val, const Damper *damper, f32 delta, f32 x) {
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

Rect BoundingBoxOfSelection(const v2 *vects, const u32 count, const b64 selMap);

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
string                    ReadEntireFile(const cstr filename);

// ===== DEBUG =====

typedef struct Logger Logger;

typedef enum { LEVEL_INFO, LEVEL_WARNING, LEVEL_ERROR, LEVEL_FATAL } LogLevel;

inline void Log(LogLevel level, cstr ctx, cstr msg, ...) {
    va_list args;
    va_start(args, msg);
    cstr logLevel = level == LEVEL_FATAL     ? "Fatal"
                    : level == LEVEL_ERROR   ? "Error"
                    : level == LEVEL_WARNING ? "Warning"
                                             : "Info";

    printf("[%s] [%s] [%s] ", __TIMESTAMP__, logLevel, ctx);
    vprintf(msg, args);
    printf("\n");
}

#if LOG_LEVEL <= 0
#define LOG_INFO(msg, ...) Log(LEVEL_INFO, __func__, msg, ##__VA_ARGS__)
#else
#define LOG_INFO(msg)
#endif

#if LOG_LEVEL <= 1
#define LOG_WARNING(msg, ...) Log(LEVEL_WARNING, __func__, msg, ##__VA_ARGS__)
#else
#define LOG_WARNING(msg)
#endif

#if LOG_LEVEL <= 2
#define LOG_ERROR(msg, ...) Log(LEVEL_ERROR, __func__, msg, ##__VA_ARGS__)
#else
#define LOG_ERROR(msg)
#endif

#if LOG_LEVEL <= 3
#define LOG_FATAL(msg, ...)                                                                        \
    {                                                                                              \
        Log(LEVEL_FATAL, __func__, msg, ##__VA_ARGS__);                                            \
        abort();                                                                                   \
    };
#else
#define LOG_FATAL(msg)
#endif