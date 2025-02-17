#pragma once
#include <cmath>
#include <cstdint>

// TODO(violeta): This doesn't crash the program on PS1, I think.
#ifdef DEBUG
#define Assert(expression) \
    if (!(expression)) {   \
        *(int *)0 = 0;     \
    }
#else
#define Assert(expression)
#endif

#define internal static
#define local_persist static
#define global_variable static

#define PI 3.14159265359f
#define TAU 6.283185307179586f

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