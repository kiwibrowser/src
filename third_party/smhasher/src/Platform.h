//-----------------------------------------------------------------------------
// Platform-specific functions and macros

#pragma once

void SetAffinity ( int cpu );

//-----------------------------------------------------------------------------
// Microsoft Visual Studio

#if defined(_MSC_VER)

#define FORCE_INLINE	__forceinline
#define	NEVER_INLINE  __declspec(noinline)

#include <stdlib.h>
#include <math.h>   // Has to be included before intrin.h or VC complains about 'ceil'
#include <intrin.h> // for __rdtsc
#include "pstdint.h"

#define ROTL32(x,y)	_rotl(x,y)
#define ROTL64(x,y)	_rotl64(x,y)
#define ROTR32(x,y)	_rotr(x,y)
#define ROTR64(x,y)	_rotr64(x,y)

#pragma warning(disable : 4127) // "conditional expression is constant" in the if()s for avalanchetest
#pragma warning(disable : 4100)
#pragma warning(disable : 4702)

#define BIG_CONSTANT(x) (x)

// RDTSC == Read Time Stamp Counter

#define rdtsc() __rdtsc()

//-----------------------------------------------------------------------------
// Other compilers

#else	//	defined(_MSC_VER)

#include <stdint.h>

#define	FORCE_INLINE inline __attribute__((always_inline))
#define	NEVER_INLINE __attribute__((noinline))

inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}

inline uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

inline uint32_t rotr32 ( uint32_t x, int8_t r )
{
  return (x >> r) | (x << (32 - r));
}

inline uint64_t rotr64 ( uint64_t x, int8_t r )
{
  return (x >> r) | (x << (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)
#define	ROTR32(x,y)	rotr32(x,y)
#define ROTR64(x,y)	rotr64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

__inline__ unsigned long long int rdtsc()
{
#ifdef __x86_64__
    unsigned int a, d;
    __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
    return (unsigned long)a | ((unsigned long)d << 32);
#elif defined(__i386__)
    unsigned long long int x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
#else
#define NO_CYCLE_COUNTER
    return 0;
#endif
}

#include <strings.h>
#define _stricmp strcasecmp

#endif	//	!defined(_MSC_VER)

//-----------------------------------------------------------------------------
