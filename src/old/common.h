#pragma once

#include <cmath>
#include <cstdint>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define UNREACHABLE (*(int*)0 = 0)

#ifdef HANDMADE_INTERNAL
#define Assert(expression) \
    if (!(expression)) {   \
        UNREACHABLE;       \
    }
#else
#define Assert(expression)
#endif

#define internal static
#define local_persist static
#define global static

#define PI 3.14159274f
#define TAU 6.28318548f

#define PI64 3.1415926535897931
#define TAU64 6.2831853071795862

#define KB(val) ((val) * 1024)
#define MB(val) (KB(val) * 1024)
#define GB(val) (MB(val) * 1024)

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef int32_t b32;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef double_t f64;
typedef float_t  f32;

struct v2 {
    f32 x, y;
};

union v2i {
    struct {
        i32 x, y;
    };
    struct {
        i32 width, height;
    };
};