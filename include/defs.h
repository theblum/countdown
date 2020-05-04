#if !defined(DEFS_H)
#define DEFS_H
/* ===========================================================================
 * File: defs.h
 * Date: 03 May 2020
 * Creator: Brian Blumberg <blum@disroot.org>
 * Notice: Copyright (c) 2020 Brian Blumberg. All Rights Reserved.
 * ===========================================================================
 */

#include <float.h>
#include <stdint.h>

#define global   static
#define internal static
#define persist  static

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef intptr_t smm;
typedef intmax_t smx;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef uintptr_t umm;
typedef uintmax_t umx;

typedef int8_t  b8;
typedef int16_t b16;
typedef int32_t b32;

typedef float  f32;
typedef double f64;

#define F32_MIN FLT_MIN
#define F32_MAX FLT_MAX
#define F32_EPSILON FLT_EPSILON

#define F64_MIN DBL_MIN
#define F64_MAX DBL_MAX
#define F64_EPSILON DBL_EPSILON

#define NANOSECS_TO_MILLISECS 0.000001f
#define MILLISECS_TO_NANOSECS 1000000
#define SECS_TO_MILLISECS 1000
#define MILLISECS_TO_SECS 0.001f

#endif
