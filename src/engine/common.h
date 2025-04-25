#pragma once

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#define export __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define export __attribute__((visibility("default")))
#else
#define export
#endif

#define internal static
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
    u32  len;
} Array;

typedef Array string;
string        String(cstr data) {
    string result;
    result.data = data;
    result.len  = (u32)strlen(data);
    return result;
}

typedef union {
    struct {
        f32 x, y;
    };
    struct {
        f32 w, h;
    };
} v2;

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
inline bool V2InRect(v2 pos, Rect rectangle);

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
