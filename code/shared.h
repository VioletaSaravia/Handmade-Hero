#pragma once
#include <cmath>
#include <cstdint>

#define internal static
#define local_persist static
#define global_variable static

#define PI 3.14159265359f
#define TAU 6.283185307179586f

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

struct v2i {
    i32 x, y;
};