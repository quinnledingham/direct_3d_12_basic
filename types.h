#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string>

#include <cstdint>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s8 b8;
typedef s32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef r32 f32;
typedef r64 f64;

#define function      static
#define internal      static
#define local_persist static
#define global        static

#define DEG2RAD 0.0174533f
#define PI      3.14159265359f
#define EPSILON 0.00001f

void *platform_malloc(u32 size);
void platform_free(void *ptr);

#define ARRAY_COUNT(n)     (sizeof(n) / sizeof(n[0]))
#define ARRAY_MALLOC(t, n) ((t*)platform_malloc(n * sizeof(t)))

union v2
{
    struct
    {
        r32 x, y;
    };
    struct
    {
        r32 u, v;
    };
    struct
    {
        r32 width, height;
    };
    r32 E[2];
};

union v2s
{
    struct
    {
        s32 x, y;
    };
    struct
    {
        s32 u, v;
    };
    struct
    {
        s32 width, height;
    };
    s32 E[2];
};

union v3
{
    struct
    {
        r32 x, y, z;
    };
    struct
    {
        r32 r, g, b;
    };
    r32 E[3];
};

union v4
{
    struct
    {
        r32 x, y, z, w;
    };
    struct
    {
        r32 r, g, b, a;
    };
    struct
    {
        v3 rgb;
        r32 alpha;
    };
    r32 E[4];
};

union quat
{
    struct
    {
        r32 x, y, z, w;
    };
    struct
    {
        v3 vector;
        f32 scalar;
    };
    r32 E[4];
};

struct m4x4
{
    r32 E[4][4];
};

#endif //TYPES_H
