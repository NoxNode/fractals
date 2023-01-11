#pragma once

#include <inttypes.h>
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;
typedef float    r32;
typedef double   r64;

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;
typedef vec2 v2;
typedef vec3 v3;

#define PI 3.14159265f

// when dealing with sizes like blocks of memory,
// these macros allow you to say stuff like
// 5 MB and 100 KB
#define KB *1024
#define MB *1024*1024
#define GB *1024*1024*1024

#define INLINE __attribute__((always_inline))
