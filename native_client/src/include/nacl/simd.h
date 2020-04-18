/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Until PNaCl supports portable vectors, PNaCl build will be reference
// impl.
#if defined(__pnacl__)
#undef VREFIMPL
#define VREFIMPL
#endif

#if !defined(ALWAYS_INLINE)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#if defined(__i686__) || defined(__x86_64__)
#define __x86__ 1
#endif

#if defined(VREFIMPL)
// compile VREFIMPLE with -std=c++0x
#elif defined(__arm__)
#include <arm_neon.h>
#elif defined(__x86__)
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>
#endif

// Note: for the reference version, __pnacl_builtin_shufflevector is
// defined instead of __builtin_shufflevector.  This is required to
// avoid clashing with the already supplied __builtin_shufflevector.
//
// In addition, the reference version implements the following classes
// that correspond to the following vector types:
//   class _i8x16   typedef int8_t _i8x16 __attribute__ ((vector_size (16)));
//   class _u8x16   typedef uint8_t _u8x16 __attribute__ ((vector_size (16)));
//   class _i16x8   typedef int16_t _i16x8 __attribute__ ((vector_size (16)));
//   class _u16x8   typedef uint16_t _u16x8 __attribute__ ((vector_size (16)));
//   class _i32x4   typedef int32_t _i32x4 __attribute__ ((vector_size (16)));
//   class _u32x4   typedef uint32_t _u32x4 __attribute__ ((vector_size (16)));
//   class _f32x4   typedef float _f32x4 __attribute__ ((vector_size (16)));
//

// Unless noted both the reference version and portable SIMD
// implementation support:
//
// operators:
//   []
//   unary operators +,-
//   ++,--                              (*)
//   +,-,*
//   /                                  float only for v1.
//   %                                  (*)
//   bitwise &,|,^,~
//   >>,<<
//   ==,!=,>,<,>=,<=
//   =
//   sizeof
//
// In addition to the operators above, the builtins:
//   __builtin_shufflevector        shuffle between one or two sources
//   __pnacl_builtin_shufflevector  same as above
//   __pnacl_builtin_load           unaligned vector load (any alignment)
//   __pnacl_builtin_store          unaligned vector store (any alignment)
//   __pnacl_builtin_min            min elements between 2 vectors
//   __pnacl_builtin_max            max elements between 2 vectors
//   __pnacl_builtin_satadd         saturated add between 2 integer vectors
//   __pnacl_builtin_satsub         saturated sub between 2 integer vectors
//   __pnacl_builtin_abs            absolute value of vector
//   __pnacl_builtin_avg            average of 2 vectors (uint8x16_t only)
//   __pnacl_builtin_reciprocal     full precision reciprocal of vector
//                                  (float only)
//   __pnacl_builtin_reciprocalsqrt full precision reciprocal sqrt of vector
//                                  (float only)
//   __pnacl_builtin_convertvector  convert float to u/int32 and vice versa
//   __pnacl_builtin_satpack        pack (narrow) fixed point pair of vectors
//                                  to saturated single vector of smaller type
//   __pnacl_builtin_signbits (*)   return sign bits of float vector input
//                                  packed into a single uint32_t.  Upper
//                                  bits set to zero.
// (*) punt for v1 ABI

#if defined(VREFIMPL)

class _i8x16;
class _u8x16;
class _i16x8;
class _u16x8;
class _i32x4;
class _u32x4;
class _f32x4;

// To reduce conversion permutations, supply a 64x2 type.  All 128 bit
// vectors should have a constructor accepting _x64x2, and all should
// supply a bit-for-bit conversion to _x64x2.  To convert between
// float32 and u/int32, use __pnacl_builtin_convertvector.
struct _x64x2 {
  uint64_t v[2];
};

class _i8x16 {
 public:
  _i8x16() { /* elements undefined */
  }
  _i8x16(int8_t v0, int8_t v1, int8_t v2, int8_t v3, int8_t v4, int8_t v5,
         int8_t v6, int8_t v7, int8_t v8, int8_t v9, int8_t v10, int8_t v11,
         int8_t v12, int8_t v13, int8_t v14, int8_t v15) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    v[4] = v4;
    v[5] = v5;
    v[6] = v6;
    v[7] = v7;
    v[8] = v8;
    v[9] = v9;
    v[10] = v10;
    v[11] = v11;
    v[12] = v12;
    v[13] = v13;
    v[14] = v14;
    v[15] = v15;
  }
  explicit _i8x16(int8_t b) {
    v[0] = v[1] = v[2] = v[3] = v[4] = v[5] = v[6] = v[7] = v[8] = v[9] =
        v[10] = v[11] = v[12] = v[13] = v[14] = v[15] = b;
  }
  explicit _i8x16(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  int8_t& operator[](int i) { return v[i]; }
  const int8_t& operator[](int i) const { return v[i]; }
  friend _i8x16 operator-(const _i8x16 a) {
    return _i8x16(-a.v[0], -a.v[1], -a.v[2], -a.v[3], -a.v[4], -a.v[5], -a.v[6],
                  -a.v[7], -a.v[8], -a.v[9], -a.v[10], -a.v[11], -a.v[12],
                  -a.v[13], -a.v[14], -a.v[15]);
  }
  friend _i8x16 operator+(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3], a.v[4] + b.v[4], a.v[5] + b.v[5],
                  a.v[6] + b.v[6], a.v[7] + b.v[7], a.v[8] + b.v[8],
                  a.v[9] + b.v[9], a.v[10] + b.v[10], a.v[11] + b.v[11],
                  a.v[12] + b.v[12], a.v[13] + b.v[13], a.v[14] + b.v[14],
                  a.v[15] + b.v[15]);
  }
  friend _i8x16 operator-(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3], a.v[4] - b.v[4], a.v[5] - b.v[5],
                  a.v[6] - b.v[6], a.v[7] - b.v[7], a.v[8] - b.v[8],
                  a.v[9] - b.v[9], a.v[10] - b.v[10], a.v[11] - b.v[11],
                  a.v[12] - b.v[12], a.v[13] - b.v[13], a.v[14] - b.v[14],
                  a.v[15] - b.v[15]);
  }
  friend _i8x16 operator*(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3], a.v[4] * b.v[4], a.v[5] * b.v[5],
                  a.v[6] * b.v[6], a.v[7] * b.v[7], a.v[8] * b.v[8],
                  a.v[9] * b.v[9], a.v[10] * b.v[10], a.v[11] * b.v[11],
                  a.v[12] * b.v[12], a.v[13] * b.v[13], a.v[14] * b.v[14],
                  a.v[15] * b.v[15]);
  }
#if defined(VREFOPTIONAL)
  friend _i8x16 operator/(const _i8x16 a, const _i8x16 b) {
    _i8x16 r;
    for (int i = 0; i < 16; i++) {
      if (b.v[i] == 0 || (a.v[i] == SCHAR_MIN && b.v[i] == -1))
        r[i] = 0;
      else
        r[i] = a.v[i] / b.v[i];
    }
    return r;
  }
  friend _i8x16 operator%(const _i8x16 a, const _i8x16 b) {
    _i8x16 r;
    for (int i = 0; i < 16; i++) {
      if (b.v[i] == 0 || (a.v[i] == SCHAR_MIN && b.v[i] == -1))
        r[i] = 0;
      else
        r[i] = a.v[i] % b.v[i];
    }
    return r;
  }
#endif
  friend _i8x16 operator<<(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] << b.v[0], a.v[1] << b.v[1], a.v[2] << b.v[2],
                  a.v[3] << b.v[3], a.v[4] << b.v[4], a.v[5] << b.v[5],
                  a.v[6] << b.v[6], a.v[7] << b.v[7], a.v[8] << b.v[8],
                  a.v[9] << b.v[9], a.v[10] << b.v[10], a.v[11] << b.v[11],
                  a.v[12] << b.v[12], a.v[13] << b.v[13], a.v[14] << b.v[14],
                  a.v[15] << b.v[15]);
  }
  friend _i8x16 operator>>(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] >> b.v[0], a.v[1] >> b.v[1], a.v[2] >> b.v[2],
                  a.v[3] >> b.v[3], a.v[4] >> b.v[4], a.v[5] >> b.v[5],
                  a.v[6] >> b.v[6], a.v[7] >> b.v[7], a.v[8] >> b.v[8],
                  a.v[9] >> b.v[9], a.v[10] >> b.v[10], a.v[11] >> b.v[11],
                  a.v[12] >> b.v[12], a.v[13] >> b.v[13], a.v[14] >> b.v[14],
                  a.v[15] >> b.v[15]);
  }
  friend _i8x16 operator~(const _i8x16 a) {
    return _i8x16(~a.v[0], ~a.v[1], ~a.v[2], ~a.v[3], ~a.v[4], ~a.v[5], ~a.v[6],
                  ~a.v[7], ~a.v[8], ~a.v[9], ~a.v[10], ~a.v[11], ~a.v[12],
                  ~a.v[13], ~a.v[14], ~a.v[15]);
  }
  friend _i8x16 operator^(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] ^ b.v[0], a.v[1] ^ b.v[1], a.v[2] ^ b.v[2],
                  a.v[3] ^ b.v[3], a.v[4] ^ b.v[4], a.v[5] ^ b.v[5],
                  a.v[6] ^ b.v[6], a.v[7] ^ b.v[7], a.v[8] ^ b.v[8],
                  a.v[9] ^ b.v[9], a.v[10] ^ b.v[10], a.v[11] ^ b.v[11],
                  a.v[12] ^ b.v[12], a.v[13] ^ b.v[13], a.v[14] ^ b.v[14],
                  a.v[15] ^ b.v[15]);
  }
  friend _i8x16 operator&(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] & b.v[0], a.v[1] & b.v[1], a.v[2] & b.v[2],
                  a.v[3] & b.v[3], a.v[4] & b.v[4], a.v[5] & b.v[5],
                  a.v[6] & b.v[6], a.v[7] & b.v[7], a.v[8] & b.v[8],
                  a.v[9] & b.v[9], a.v[10] & b.v[10], a.v[11] & b.v[11],
                  a.v[12] & b.v[12], a.v[13] & b.v[13], a.v[14] & b.v[14],
                  a.v[15] & b.v[15]);
  }
  friend _i8x16 operator|(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] | b.v[0], a.v[1] | b.v[1], a.v[2] | b.v[2],
                  a.v[3] | b.v[3], a.v[4] | b.v[4], a.v[5] | b.v[5],
                  a.v[6] | b.v[6], a.v[7] | b.v[7], a.v[8] | b.v[8],
                  a.v[9] | b.v[9], a.v[10] | b.v[10], a.v[11] | b.v[11],
                  a.v[12] | b.v[12], a.v[13] | b.v[13], a.v[14] | b.v[14],
                  a.v[15] | b.v[15]);
  }
  friend _i8x16 operator==(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0,
                  a.v[4] == b.v[4] ? -1 : 0, a.v[5] == b.v[5] ? -1 : 0,
                  a.v[6] == b.v[6] ? -1 : 0, a.v[7] == b.v[7] ? -1 : 0,
                  a.v[8] == b.v[8] ? -1 : 0, a.v[9] == b.v[9] ? -1 : 0,
                  a.v[10] == b.v[10] ? -1 : 0, a.v[11] == b.v[11] ? -1 : 0,
                  a.v[12] == b.v[12] ? -1 : 0, a.v[13] == b.v[13] ? -1 : 0,
                  a.v[14] == b.v[14] ? -1 : 0, a.v[15] == b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator!=(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0,
                  a.v[4] != b.v[4] ? -1 : 0, a.v[5] != b.v[5] ? -1 : 0,
                  a.v[6] != b.v[6] ? -1 : 0, a.v[7] != b.v[7] ? -1 : 0,
                  a.v[8] != b.v[8] ? -1 : 0, a.v[9] != b.v[9] ? -1 : 0,
                  a.v[10] != b.v[10] ? -1 : 0, a.v[11] != b.v[11] ? -1 : 0,
                  a.v[12] != b.v[12] ? -1 : 0, a.v[13] != b.v[13] ? -1 : 0,
                  a.v[14] != b.v[14] ? -1 : 0, a.v[15] != b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator<(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0,
                  a.v[4] < b.v[4] ? -1 : 0, a.v[5] < b.v[5] ? -1 : 0,
                  a.v[6] < b.v[6] ? -1 : 0, a.v[7] < b.v[7] ? -1 : 0,
                  a.v[8] < b.v[8] ? -1 : 0, a.v[9] < b.v[9] ? -1 : 0,
                  a.v[10] < b.v[10] ? -1 : 0, a.v[11] < b.v[11] ? -1 : 0,
                  a.v[12] < b.v[12] ? -1 : 0, a.v[13] < b.v[13] ? -1 : 0,
                  a.v[14] < b.v[14] ? -1 : 0, a.v[15] < b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator<=(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0,
                  a.v[4] <= b.v[4] ? -1 : 0, a.v[5] <= b.v[5] ? -1 : 0,
                  a.v[6] <= b.v[6] ? -1 : 0, a.v[7] <= b.v[7] ? -1 : 0,
                  a.v[8] <= b.v[8] ? -1 : 0, a.v[9] <= b.v[9] ? -1 : 0,
                  a.v[10] <= b.v[10] ? -1 : 0, a.v[11] <= b.v[11] ? -1 : 0,
                  a.v[12] <= b.v[12] ? -1 : 0, a.v[13] <= b.v[13] ? -1 : 0,
                  a.v[14] <= b.v[14] ? -1 : 0, a.v[15] <= b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator>(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0,
                  a.v[4] > b.v[4] ? -1 : 0, a.v[5] > b.v[5] ? -1 : 0,
                  a.v[6] > b.v[6] ? -1 : 0, a.v[7] > b.v[7] ? -1 : 0,
                  a.v[8] > b.v[8] ? -1 : 0, a.v[9] > b.v[9] ? -1 : 0,
                  a.v[10] > b.v[10] ? -1 : 0, a.v[11] > b.v[11] ? -1 : 0,
                  a.v[12] > b.v[12] ? -1 : 0, a.v[13] > b.v[13] ? -1 : 0,
                  a.v[14] > b.v[14] ? -1 : 0, a.v[15] > b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator>=(const _i8x16 a, const _i8x16 b) {
    return _i8x16(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0,
                  a.v[4] >= b.v[4] ? -1 : 0, a.v[5] >= b.v[5] ? -1 : 0,
                  a.v[6] >= b.v[6] ? -1 : 0, a.v[7] >= b.v[7] ? -1 : 0,
                  a.v[8] >= b.v[8] ? -1 : 0, a.v[9] >= b.v[9] ? -1 : 0,
                  a.v[10] >= b.v[10] ? -1 : 0, a.v[11] >= b.v[11] ? -1 : 0,
                  a.v[12] >= b.v[12] ? -1 : 0, a.v[13] >= b.v[13] ? -1 : 0,
                  a.v[14] >= b.v[14] ? -1 : 0, a.v[15] >= b.v[15] ? -1 : 0);
  }
  friend _i8x16 __pnacl_builtin_shufflevector(
      const _i8x16 a, const _i8x16 b, const int i0, const int i1, const int i2,
      const int i3, const int i4, const int i5, const int i6, const int i7,
      const int i8, const int i9, const int i10, const int i11, const int i12,
      const int i13, const int i14, const int i15) {
    return _i8x16(
        i0 < 16 ? a.v[i0] : b.v[i0 - 16], i1 < 16 ? a.v[i1] : b.v[i1 - 16],
        i2 < 16 ? a.v[i2] : b.v[i2 - 16], i3 < 16 ? a.v[i3] : b.v[i3 - 16],
        i4 < 16 ? a.v[i4] : b.v[i4 - 16], i5 < 16 ? a.v[i5] : b.v[i5 - 16],
        i6 < 16 ? a.v[i6] : b.v[i6 - 16], i7 < 16 ? a.v[i7] : b.v[i7 - 16],
        i8 < 16 ? a.v[i8] : b.v[i8 - 16], i9 < 16 ? a.v[i9] : b.v[i9 - 16],
        i10 < 16 ? a.v[i10] : b.v[i10 - 16],
        i11 < 16 ? a.v[i11] : b.v[i11 - 16],
        i12 < 16 ? a.v[i12] : b.v[i12 - 16],
        i13 < 16 ? a.v[i13] : b.v[i13 - 16],
        i14 < 16 ? a.v[i14] : b.v[i14 - 16],
        i15 < 16 ? a.v[i15] : b.v[i15 - 16]);
  }
  friend _i8x16 __pnacl_builtin_load(_i8x16* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _i8x16 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _i8x16 __pnacl_builtin_min(const _i8x16 a, const _i8x16 b) {
    return _i8x16(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3],
        a.v[4] < b.v[4] ? a.v[4] : b.v[4], a.v[5] < b.v[5] ? a.v[5] : b.v[5],
        a.v[6] < b.v[6] ? a.v[6] : b.v[6], a.v[7] < b.v[7] ? a.v[7] : b.v[7],
        a.v[8] < b.v[8] ? a.v[8] : b.v[8], a.v[9] < b.v[9] ? a.v[9] : b.v[9],
        a.v[10] < b.v[10] ? a.v[10] : b.v[10],
        a.v[11] < b.v[11] ? a.v[11] : b.v[11],
        a.v[12] < b.v[12] ? a.v[12] : b.v[12],
        a.v[13] < b.v[13] ? a.v[13] : b.v[13],
        a.v[14] < b.v[14] ? a.v[14] : b.v[14],
        a.v[15] < b.v[15] ? a.v[15] : b.v[15]);
  }
  friend _i8x16 __pnacl_builtin_max(const _i8x16 a, const _i8x16 b) {
    return _i8x16(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3],
        a.v[4] > b.v[4] ? a.v[4] : b.v[4], a.v[5] > b.v[5] ? a.v[5] : b.v[5],
        a.v[6] > b.v[6] ? a.v[6] : b.v[6], a.v[7] > b.v[7] ? a.v[7] : b.v[7],
        a.v[8] > b.v[8] ? a.v[8] : b.v[8], a.v[9] > b.v[9] ? a.v[9] : b.v[9],
        a.v[10] > b.v[10] ? a.v[10] : b.v[10],
        a.v[11] > b.v[11] ? a.v[11] : b.v[11],
        a.v[12] > b.v[12] ? a.v[12] : b.v[12],
        a.v[13] > b.v[13] ? a.v[13] : b.v[13],
        a.v[14] > b.v[14] ? a.v[14] : b.v[14],
        a.v[15] > b.v[15] ? a.v[15] : b.v[15]);
  }
  friend _i8x16 __pnacl_builtin_satadd(const _i8x16 a, const _i8x16 b) {
    return _i8x16(satadd_i8(a.v[0], b.v[0]), satadd_i8(a.v[1], b.v[1]),
                  satadd_i8(a.v[2], b.v[2]), satadd_i8(a.v[3], b.v[3]),
                  satadd_i8(a.v[4], b.v[4]), satadd_i8(a.v[5], b.v[5]),
                  satadd_i8(a.v[6], b.v[6]), satadd_i8(a.v[7], b.v[7]),
                  satadd_i8(a.v[8], b.v[8]), satadd_i8(a.v[9], b.v[9]),
                  satadd_i8(a.v[10], b.v[10]), satadd_i8(a.v[11], b.v[11]),
                  satadd_i8(a.v[12], b.v[12]), satadd_i8(a.v[13], b.v[13]),
                  satadd_i8(a.v[14], b.v[14]), satadd_i8(a.v[15], b.v[15]));
  }
  friend _i8x16 __pnacl_builtin_satsub(const _i8x16 a, const _i8x16 b) {
    return _i8x16(satsub_i8(a.v[0], b.v[0]), satsub_i8(a.v[1], b.v[1]),
                  satsub_i8(a.v[2], b.v[2]), satsub_i8(a.v[3], b.v[3]),
                  satsub_i8(a.v[4], b.v[4]), satsub_i8(a.v[5], b.v[5]),
                  satsub_i8(a.v[6], b.v[6]), satsub_i8(a.v[7], b.v[7]),
                  satsub_i8(a.v[8], b.v[8]), satsub_i8(a.v[9], b.v[9]),
                  satsub_i8(a.v[10], b.v[10]), satsub_i8(a.v[11], b.v[11]),
                  satsub_i8(a.v[12], b.v[12]), satsub_i8(a.v[13], b.v[13]),
                  satsub_i8(a.v[14], b.v[14]), satsub_i8(a.v[15], b.v[15]));
  }
  friend _i8x16 __pnacl_builtin_abs(const _i8x16 a) {
    return _i8x16(abs(a.v[0]), abs(a.v[1]), abs(a.v[2]), abs(a.v[3]),
                  abs(a.v[4]), abs(a.v[5]), abs(a.v[6]), abs(a.v[7]),
                  abs(a.v[8]), abs(a.v[9]), abs(a.v[10]), abs(a.v[11]),
                  abs(a.v[12]), abs(a.v[13]), abs(a.v[14]), abs(a.v[15]));
  }

 private:
  static int8_t satadd_i8(const int8_t a, const int8_t b) {
    int16_t t = static_cast<int16_t>(a) + static_cast<int16_t>(b);
    if (t > SCHAR_MAX) return SCHAR_MAX;
    if (t < SCHAR_MIN) return SCHAR_MIN;
    return static_cast<int8_t>(t);
  }
  static int8_t satsub_i8(const int8_t a, const int8_t b) {
    int16_t t = static_cast<int16_t>(a) - static_cast<int16_t>(b);
    if (t > SCHAR_MAX) return SCHAR_MAX;
    if (t < SCHAR_MIN) return SCHAR_MIN;
    return static_cast<int8_t>(t);
  }
  union {
    int8_t v[16];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

class _u8x16 {
 public:
  _u8x16() { /* leave elements undefined */
  }
  _u8x16(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5,
         uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9, uint8_t v10,
         uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    v[4] = v4;
    v[5] = v5;
    v[6] = v6;
    v[7] = v7;
    v[8] = v8;
    v[9] = v9;
    v[10] = v10;
    v[11] = v11;
    v[12] = v12;
    v[13] = v13;
    v[14] = v14;
    v[15] = v15;
  }
  explicit _u8x16(uint8_t b) {
    v[0] = v[1] = v[2] = v[3] = v[4] = v[5] = v[6] = v[7] = v[8] = v[9] =
        v[10] = v[11] = v[12] = v[13] = v[14] = v[15] = b;
  }
  explicit _u8x16(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  uint8_t& operator[](int i) { return v[i]; }
  const uint8_t& operator[](int i) const { return v[i]; }
  friend _u8x16 operator+(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3], a.v[4] + b.v[4], a.v[5] + b.v[5],
                  a.v[6] + b.v[6], a.v[7] + b.v[7], a.v[8] + b.v[8],
                  a.v[9] + b.v[9], a.v[10] + b.v[10], a.v[11] + b.v[11],
                  a.v[12] + b.v[12], a.v[13] + b.v[13], a.v[14] + b.v[14],
                  a.v[15] + b.v[15]);
  }
  friend _u8x16 operator-(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3], a.v[4] - b.v[4], a.v[5] - b.v[5],
                  a.v[6] - b.v[6], a.v[7] - b.v[7], a.v[8] - b.v[8],
                  a.v[9] - b.v[9], a.v[10] - b.v[10], a.v[11] - b.v[11],
                  a.v[12] - b.v[12], a.v[13] - b.v[13], a.v[14] - b.v[14],
                  a.v[15] - b.v[15]);
  }
  friend _u8x16 operator*(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3], a.v[4] * b.v[4], a.v[5] * b.v[5],
                  a.v[6] * b.v[6], a.v[7] * b.v[7], a.v[8] * b.v[8],
                  a.v[9] * b.v[9], a.v[10] * b.v[10], a.v[11] * b.v[11],
                  a.v[12] * b.v[12], a.v[13] * b.v[13], a.v[14] * b.v[14],
                  a.v[15] * b.v[15]);
  }
#if defined(VREFOPTIONAL)
  friend _u8x16 operator/(const _u8x16 a, const _u8x16 b) {
    _u8x16 r;
    for (int i = 0; i < 16; i++) {
      if (b.v[i] == 0)
        r[i] = 0;
      else
        r[i] = a.v[i] / b.v[i];
    }
    return r;
  }
  friend _u8x16 operator%(const _u8x16 a, const _u8x16 b) {
    _u8x16 r;
    for (int i = 0; i < 16; i++) {
      if (b.v[i] == 0)
        r[i] = 0;
      else
        r[i] = a.v[i] % b.v[i];
    }
    return r;
  }
#endif
  friend _u8x16 operator<<(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] << b.v[0], a.v[1] << b.v[1], a.v[2] << b.v[2],
                  a.v[3] << b.v[3], a.v[4] << b.v[4], a.v[5] << b.v[5],
                  a.v[6] << b.v[6], a.v[7] << b.v[7], a.v[8] << b.v[8],
                  a.v[9] << b.v[9], a.v[10] << b.v[10], a.v[11] << b.v[11],
                  a.v[12] << b.v[12], a.v[13] << b.v[13], a.v[14] << b.v[14],
                  a.v[15] << b.v[15]);
  }
  friend _u8x16 operator>>(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] >> b.v[0], a.v[1] >> b.v[1], a.v[2] >> b.v[2],
                  a.v[3] >> b.v[3], a.v[4] >> b.v[4], a.v[5] >> b.v[5],
                  a.v[6] >> b.v[6], a.v[7] >> b.v[7], a.v[8] >> b.v[8],
                  a.v[9] >> b.v[9], a.v[10] >> b.v[10], a.v[11] >> b.v[11],
                  a.v[12] >> b.v[12], a.v[13] >> b.v[13], a.v[14] >> b.v[14],
                  a.v[15] >> b.v[15]);
  }
  friend _u8x16 operator~(const _u8x16 a) {
    return _u8x16(~a.v[0], ~a.v[1], ~a.v[2], ~a.v[3], ~a.v[4], ~a.v[5], ~a.v[6],
                  ~a.v[7], ~a.v[8], ~a.v[9], ~a.v[10], ~a.v[11], ~a.v[12],
                  ~a.v[13], ~a.v[14], ~a.v[15]);
  }
  friend _u8x16 operator^(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] ^ b.v[0], a.v[1] ^ b.v[1], a.v[2] ^ b.v[2],
                  a.v[3] ^ b.v[3], a.v[4] ^ b.v[4], a.v[5] ^ b.v[5],
                  a.v[6] ^ b.v[6], a.v[7] ^ b.v[7], a.v[8] ^ b.v[8],
                  a.v[9] ^ b.v[9], a.v[10] ^ b.v[10], a.v[11] ^ b.v[11],
                  a.v[12] ^ b.v[12], a.v[13] ^ b.v[13], a.v[14] ^ b.v[14],
                  a.v[15] ^ b.v[15]);
  }
  friend _u8x16 operator&(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] & b.v[0], a.v[1] & b.v[1], a.v[2] & b.v[2],
                  a.v[3] & b.v[3], a.v[4] & b.v[4], a.v[5] & b.v[5],
                  a.v[6] & b.v[6], a.v[7] & b.v[7], a.v[8] & b.v[8],
                  a.v[9] & b.v[9], a.v[10] & b.v[10], a.v[11] & b.v[11],
                  a.v[12] & b.v[12], a.v[13] & b.v[13], a.v[14] & b.v[14],
                  a.v[15] & b.v[15]);
  }
  friend _u8x16 operator|(const _u8x16 a, const _u8x16 b) {
    return _u8x16(a.v[0] | b.v[0], a.v[1] | b.v[1], a.v[2] | b.v[2],
                  a.v[3] | b.v[3], a.v[4] | b.v[4], a.v[5] | b.v[5],
                  a.v[6] | b.v[6], a.v[7] | b.v[7], a.v[8] | b.v[8],
                  a.v[9] | b.v[9], a.v[10] | b.v[10], a.v[11] | b.v[11],
                  a.v[12] | b.v[12], a.v[13] | b.v[13], a.v[14] | b.v[14],
                  a.v[15] | b.v[15]);
  }
  friend _i8x16 operator==(const _u8x16 a, const _u8x16 b) {
    return _i8x16(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0,
                  a.v[4] == b.v[4] ? -1 : 0, a.v[5] == b.v[5] ? -1 : 0,
                  a.v[6] == b.v[6] ? -1 : 0, a.v[7] == b.v[7] ? -1 : 0,
                  a.v[8] == b.v[8] ? -1 : 0, a.v[9] == b.v[9] ? -1 : 0,
                  a.v[10] == b.v[10] ? -1 : 0, a.v[11] == b.v[11] ? -1 : 0,
                  a.v[12] == b.v[12] ? -1 : 0, a.v[13] == b.v[13] ? -1 : 0,
                  a.v[14] == b.v[14] ? -1 : 0, a.v[15] == b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator!=(const _u8x16 a, const _u8x16 b) {
    return _i8x16(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0,
                  a.v[4] != b.v[4] ? -1 : 0, a.v[5] != b.v[5] ? -1 : 0,
                  a.v[6] != b.v[6] ? -1 : 0, a.v[7] != b.v[7] ? -1 : 0,
                  a.v[8] != b.v[8] ? -1 : 0, a.v[9] != b.v[9] ? -1 : 0,
                  a.v[10] != b.v[10] ? -1 : 0, a.v[11] != b.v[11] ? -1 : 0,
                  a.v[12] != b.v[12] ? -1 : 0, a.v[13] != b.v[13] ? -1 : 0,
                  a.v[14] != b.v[14] ? -1 : 0, a.v[15] != b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator<(const _u8x16 a, const _u8x16 b) {
    return _i8x16(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0,
                  a.v[4] < b.v[4] ? -1 : 0, a.v[5] < b.v[5] ? -1 : 0,
                  a.v[6] < b.v[6] ? -1 : 0, a.v[7] < b.v[7] ? -1 : 0,
                  a.v[8] < b.v[8] ? -1 : 0, a.v[9] < b.v[9] ? -1 : 0,
                  a.v[10] < b.v[10] ? -1 : 0, a.v[11] < b.v[11] ? -1 : 0,
                  a.v[12] < b.v[12] ? -1 : 0, a.v[13] < b.v[13] ? -1 : 0,
                  a.v[14] < b.v[14] ? -1 : 0, a.v[15] < b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator<=(const _u8x16 a, const _u8x16 b) {
    return _i8x16(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0,
                  a.v[4] <= b.v[4] ? -1 : 0, a.v[5] <= b.v[5] ? -1 : 0,
                  a.v[6] <= b.v[6] ? -1 : 0, a.v[7] <= b.v[7] ? -1 : 0,
                  a.v[8] <= b.v[8] ? -1 : 0, a.v[9] <= b.v[9] ? -1 : 0,
                  a.v[10] <= b.v[10] ? -1 : 0, a.v[11] <= b.v[11] ? -1 : 0,
                  a.v[12] <= b.v[12] ? -1 : 0, a.v[13] <= b.v[13] ? -1 : 0,
                  a.v[14] <= b.v[14] ? -1 : 0, a.v[15] <= b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator>(const _u8x16 a, const _u8x16 b) {
    return _i8x16(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0,
                  a.v[4] > b.v[4] ? -1 : 0, a.v[5] > b.v[5] ? -1 : 0,
                  a.v[6] > b.v[6] ? -1 : 0, a.v[7] > b.v[7] ? -1 : 0,
                  a.v[8] > b.v[8] ? -1 : 0, a.v[9] > b.v[9] ? -1 : 0,
                  a.v[10] > b.v[10] ? -1 : 0, a.v[11] > b.v[11] ? -1 : 0,
                  a.v[12] > b.v[12] ? -1 : 0, a.v[13] > b.v[13] ? -1 : 0,
                  a.v[14] > b.v[14] ? -1 : 0, a.v[15] > b.v[15] ? -1 : 0);
  }
  friend _i8x16 operator>=(const _u8x16 a, const _u8x16 b) {
    return _i8x16(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0,
                  a.v[4] >= b.v[4] ? -1 : 0, a.v[5] >= b.v[5] ? -1 : 0,
                  a.v[6] >= b.v[6] ? -1 : 0, a.v[7] >= b.v[7] ? -1 : 0,
                  a.v[8] >= b.v[8] ? -1 : 0, a.v[9] >= b.v[9] ? -1 : 0,
                  a.v[10] >= b.v[10] ? -1 : 0, a.v[11] >= b.v[11] ? -1 : 0,
                  a.v[12] >= b.v[12] ? -1 : 0, a.v[13] >= b.v[13] ? -1 : 0,
                  a.v[14] >= b.v[14] ? -1 : 0, a.v[15] >= b.v[15] ? -1 : 0);
  }
  friend _u8x16 __pnacl_builtin_shufflevector(
      const _u8x16 a, const _u8x16 b, const int i0, const int i1, const int i2,
      const int i3, const int i4, const int i5, const int i6, const int i7,
      const int i8, const int i9, const int i10, const int i11, const int i12,
      const int i13, const int i14, const int i15) {
    return _u8x16(
        i0 < 16 ? a.v[i0] : b.v[i0 - 16], i1 < 16 ? a.v[i1] : b.v[i1 - 16],
        i2 < 16 ? a.v[i2] : b.v[i2 - 16], i3 < 16 ? a.v[i3] : b.v[i3 - 16],
        i4 < 16 ? a.v[i4] : b.v[i4 - 16], i5 < 16 ? a.v[i5] : b.v[i5 - 16],
        i6 < 16 ? a.v[i6] : b.v[i6 - 16], i7 < 16 ? a.v[i7] : b.v[i7 - 16],
        i8 < 16 ? a.v[i8] : b.v[i8 - 16], i9 < 16 ? a.v[i9] : b.v[i9 - 16],
        i10 < 16 ? a.v[i10] : b.v[i10 - 16],
        i11 < 16 ? a.v[i11] : b.v[i11 - 16],
        i12 < 16 ? a.v[i12] : b.v[i12 - 16],
        i13 < 16 ? a.v[i13] : b.v[i13 - 16],
        i14 < 16 ? a.v[i14] : b.v[i14 - 16],
        i15 < 16 ? a.v[i15] : b.v[i15 - 16]);
  }
  friend _u8x16 __pnacl_builtin_load(_u8x16* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _u8x16 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _u8x16 __pnacl_builtin_min(const _u8x16 a, const _u8x16 b) {
    return _u8x16(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3],
        a.v[4] < b.v[4] ? a.v[4] : b.v[4], a.v[5] < b.v[5] ? a.v[5] : b.v[5],
        a.v[6] < b.v[6] ? a.v[6] : b.v[6], a.v[7] < b.v[7] ? a.v[7] : b.v[7],
        a.v[8] < b.v[8] ? a.v[8] : b.v[8], a.v[9] < b.v[9] ? a.v[9] : b.v[9],
        a.v[10] < b.v[10] ? a.v[10] : b.v[10],
        a.v[11] < b.v[11] ? a.v[11] : b.v[11],
        a.v[12] < b.v[12] ? a.v[12] : b.v[12],
        a.v[13] < b.v[13] ? a.v[13] : b.v[13],
        a.v[14] < b.v[14] ? a.v[14] : b.v[14],
        a.v[15] < b.v[15] ? a.v[15] : b.v[15]);
  }
  friend _u8x16 __pnacl_builtin_max(const _u8x16 a, const _u8x16 b) {
    return _u8x16(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3],
        a.v[4] > b.v[4] ? a.v[4] : b.v[4], a.v[5] > b.v[5] ? a.v[5] : b.v[5],
        a.v[6] > b.v[6] ? a.v[6] : b.v[6], a.v[7] > b.v[7] ? a.v[7] : b.v[7],
        a.v[8] > b.v[8] ? a.v[8] : b.v[8], a.v[9] > b.v[9] ? a.v[9] : b.v[9],
        a.v[10] > b.v[10] ? a.v[10] : b.v[10],
        a.v[11] > b.v[11] ? a.v[11] : b.v[11],
        a.v[12] > b.v[12] ? a.v[12] : b.v[12],
        a.v[13] > b.v[13] ? a.v[13] : b.v[13],
        a.v[14] > b.v[14] ? a.v[14] : b.v[14],
        a.v[15] > b.v[15] ? a.v[15] : b.v[15]);
  }
  friend _u8x16 __pnacl_builtin_satadd(const _u8x16 a, const _u8x16 b) {
    return _u8x16(satadd_u8(a.v[0], b.v[0]), satadd_u8(a.v[1], b.v[1]),
                  satadd_u8(a.v[2], b.v[2]), satadd_u8(a.v[3], b.v[3]),
                  satadd_u8(a.v[4], b.v[4]), satadd_u8(a.v[5], b.v[5]),
                  satadd_u8(a.v[6], b.v[6]), satadd_u8(a.v[7], b.v[7]),
                  satadd_u8(a.v[8], b.v[8]), satadd_u8(a.v[9], b.v[9]),
                  satadd_u8(a.v[10], b.v[10]), satadd_u8(a.v[11], b.v[11]),
                  satadd_u8(a.v[12], b.v[12]), satadd_u8(a.v[13], b.v[13]),
                  satadd_u8(a.v[14], b.v[14]), satadd_u8(a.v[15], b.v[15]));
  }
  friend _u8x16 __pnacl_builtin_satsub(const _u8x16 a, const _u8x16 b) {
    return _u8x16(satsub_u8(a.v[0], b.v[0]), satsub_u8(a.v[1], b.v[1]),
                  satsub_u8(a.v[2], b.v[2]), satsub_u8(a.v[3], b.v[3]),
                  satsub_u8(a.v[4], b.v[4]), satsub_u8(a.v[5], b.v[5]),
                  satsub_u8(a.v[6], b.v[6]), satsub_u8(a.v[7], b.v[7]),
                  satsub_u8(a.v[8], b.v[8]), satsub_u8(a.v[9], b.v[9]),
                  satsub_u8(a.v[10], b.v[10]), satsub_u8(a.v[11], b.v[11]),
                  satsub_u8(a.v[12], b.v[12]), satsub_u8(a.v[13], b.v[13]),
                  satsub_u8(a.v[14], b.v[14]), satsub_u8(a.v[15], b.v[15]));
  }
  friend _u8x16 __pnacl_builtin_avg(const _u8x16 a, const _u8x16 b) {
    return _u8x16(avg_u8(a.v[0], b.v[0]), avg_u8(a.v[1], b.v[1]),
                  avg_u8(a.v[2], b.v[2]), avg_u8(a.v[3], b.v[3]),
                  avg_u8(a.v[4], b.v[4]), avg_u8(a.v[5], b.v[5]),
                  avg_u8(a.v[6], b.v[6]), avg_u8(a.v[7], b.v[7]),
                  avg_u8(a.v[8], b.v[8]), avg_u8(a.v[9], b.v[9]),
                  avg_u8(a.v[10], b.v[10]), avg_u8(a.v[11], b.v[11]),
                  avg_u8(a.v[12], b.v[12]), avg_u8(a.v[13], b.v[13]),
                  avg_u8(a.v[14], b.v[14]), avg_u8(a.v[15], b.v[15]));
  }

 private:
  static uint8_t satadd_u8(const uint8_t a, const uint8_t b) {
    int16_t t = static_cast<int16_t>(a) + static_cast<int16_t>(b);
    if (t < 0) return 0;
    if (t > UCHAR_MAX) return UCHAR_MAX;
    return static_cast<uint8_t>(t);
  }
  static uint8_t satsub_u8(const uint8_t a, const uint8_t b) {
    int16_t t = static_cast<int16_t>(a) - static_cast<int16_t>(b);
    if (t < 0) return 0;
    if (t > UCHAR_MAX) return UCHAR_MAX;
    return static_cast<uint8_t>(t);
  }

  static int16_t sat_addubsw(uint8_t a0, int8_t b0, uint8_t a1, int8_t b1) {
    int32_t t = static_cast<int32_t>(a0) * static_cast<int32_t>(b0) +
                static_cast<int32_t>(a1) * static_cast<int32_t>(b1);
    if (t > SHRT_MAX) return SHRT_MAX;
    if (t < SHRT_MIN) return SHRT_MIN;
    return static_cast<int16_t>(t);
  }

  static uint8_t avg_u8(uint8_t a, uint8_t b) {
    return (uint8_t)(((uint16_t)a + (uint16_t)b + 1) >> 1);
  }
  union {
    uint8_t v[16];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

class _i16x8 {
 public:
  _i16x8() { /* elements undefined */
  }
  _i16x8(int16_t v0, int16_t v1, int16_t v2, int16_t v3, int16_t v4, int16_t v5,
         int16_t v6, int16_t v7) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    v[4] = v4;
    v[5] = v5;
    v[6] = v6;
    v[7] = v7;
  }
  explicit _i16x8(int32_t b) {
    v[0] = v[1] = v[2] = v[3] = v[4] = v[5] = v[6] = v[7] = b;
  }
  explicit _i16x8(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  int16_t& operator[](int i) { return v[i]; }
  const int16_t& operator[](int i) const { return v[i]; }
  friend _i16x8 operator-(const _i16x8 a) {
    return _i16x8(-a.v[0], -a.v[1], -a.v[2], -a.v[3], -a.v[4], -a.v[5], -a.v[6],
                  -a.v[7]);
  }
  friend _i16x8 operator+(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3], a.v[4] + b.v[4], a.v[5] + b.v[5],
                  a.v[6] + b.v[6], a.v[7] + b.v[7]);
  }
  friend _i16x8 operator-(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3], a.v[4] - b.v[4], a.v[5] - b.v[5],
                  a.v[6] - b.v[6], a.v[7] - b.v[7]);
  }
  friend _i16x8 operator*(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3], a.v[4] * b.v[4], a.v[5] * b.v[5],
                  a.v[6] * b.v[6], a.v[7] * b.v[7]);
  }
#if defined(VREFOPTIONAL)
  friend _i16x8 operator/(const _i16x8 a, const _i16x8 b) {
    _i16x8 r;
    for (int i = 0; i < 8; i++) {
      if (b.v[i] == 0 || (a.v[i] == SHRT_MIN && b.v[i] == -1))
        r[i] = 0;
      else
        r[i] = a.v[i] / b.v[i];
    }
    return r;
  }
  friend _i16x8 operator%(const _i16x8 a, const _i16x8 b) {
    _i16x8 r;
    for (int i = 0; i < 8; i++) {
      if (b.v[i] == 0 || (a.v[i] == SHRT_MIN && b.v[i] == -1))
        r[i] = 0;
      else
        r[i] = a.v[i] % b.v[i];
    }
    return r;
  }
#endif
  friend _i16x8 operator<<(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] << b.v[0], a.v[1] << b.v[1], a.v[2] << b.v[2],
                  a.v[3] << b.v[3], a.v[4] << b.v[4], a.v[5] << b.v[5],
                  a.v[6] << b.v[6], a.v[7] << b.v[7]);
  }
  friend _i16x8 operator>>(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] >> b.v[0], a.v[1] >> b.v[1], a.v[2] >> b.v[2],
                  a.v[3] >> b.v[3], a.v[4] >> b.v[4], a.v[5] >> b.v[5],
                  a.v[6] >> b.v[6], a.v[7] >> b.v[7]);
  }
  friend _i16x8 operator~(const _i16x8 a) {
    return _i16x8(~a.v[0], ~a.v[1], ~a.v[2], ~a.v[3], ~a.v[4], ~a.v[5], ~a.v[6],
                  ~a.v[7]);
  }
  friend _i16x8 operator^(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] ^ b.v[0], a.v[1] ^ b.v[1], a.v[2] ^ b.v[2],
                  a.v[3] ^ b.v[3], a.v[4] ^ b.v[4], a.v[5] ^ b.v[5],
                  a.v[6] ^ b.v[6], a.v[7] ^ b.v[7]);
  }
  friend _i16x8 operator&(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] & b.v[0], a.v[1] & b.v[1], a.v[2] & b.v[2],
                  a.v[3] & b.v[3], a.v[4] & b.v[4], a.v[5] & b.v[5],
                  a.v[6] & b.v[6], a.v[7] & b.v[7]);
  }
  friend _i16x8 operator|(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] | b.v[0], a.v[1] | b.v[1], a.v[2] | b.v[2],
                  a.v[3] | b.v[3], a.v[4] | b.v[4], a.v[5] | b.v[5],
                  a.v[6] | b.v[6], a.v[7] | b.v[7]);
  }
  friend _i16x8 operator==(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0,
                  a.v[4] == b.v[4] ? -1 : 0, a.v[5] == b.v[5] ? -1 : 0,
                  a.v[6] == b.v[6] ? -1 : 0, a.v[7] == b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator!=(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0,
                  a.v[4] != b.v[4] ? -1 : 0, a.v[5] != b.v[5] ? -1 : 0,
                  a.v[6] != b.v[6] ? -1 : 0, a.v[7] != b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator<(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0,
                  a.v[4] < b.v[4] ? -1 : 0, a.v[5] < b.v[5] ? -1 : 0,
                  a.v[6] < b.v[6] ? -1 : 0, a.v[7] < b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator<=(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0,
                  a.v[4] <= b.v[4] ? -1 : 0, a.v[5] <= b.v[5] ? -1 : 0,
                  a.v[6] <= b.v[6] ? -1 : 0, a.v[7] <= b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator>(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0,
                  a.v[4] > b.v[4] ? -1 : 0, a.v[5] > b.v[5] ? -1 : 0,
                  a.v[6] > b.v[6] ? -1 : 0, a.v[7] > b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator>=(const _i16x8 a, const _i16x8 b) {
    return _i16x8(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0,
                  a.v[4] >= b.v[4] ? -1 : 0, a.v[5] >= b.v[5] ? -1 : 0,
                  a.v[6] >= b.v[6] ? -1 : 0, a.v[7] >= b.v[7] ? -1 : 0);
  }
  friend _i16x8 __pnacl_builtin_shufflevector(const _i16x8 a, const _i16x8 b,
                                              const int i0, const int i1,
                                              const int i2, const int i3,
                                              const int i4, const int i5,
                                              const int i6, const int i7) {
    return _i16x8(
        i0 < 8 ? a.v[i0] : b.v[i0 - 8], i1 < 8 ? a.v[i1] : b.v[i1 - 8],
        i2 < 8 ? a.v[i2] : b.v[i2 - 8], i3 < 8 ? a.v[i3] : b.v[i3 - 8],
        i4 < 8 ? a.v[i4] : b.v[i4 - 8], i5 < 8 ? a.v[i5] : b.v[i5 - 8],
        i6 < 8 ? a.v[i6] : b.v[i6 - 8], i7 < 8 ? a.v[i7] : b.v[i7 - 8]);
  }
  friend _i16x8 __pnacl_builtin_load(_i16x8* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _i16x8 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _i16x8 __pnacl_builtin_min(const _i16x8 a, const _i16x8 b) {
    return _i16x8(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3],
        a.v[4] < b.v[4] ? a.v[4] : b.v[4], a.v[5] < b.v[5] ? a.v[5] : b.v[5],
        a.v[6] < b.v[6] ? a.v[6] : b.v[6], a.v[7] < b.v[7] ? a.v[7] : b.v[7]);
  }
  friend _i16x8 __pnacl_builtin_max(const _i16x8 a, const _i16x8 b) {
    return _i16x8(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3],
        a.v[4] > b.v[4] ? a.v[4] : b.v[4], a.v[5] > b.v[5] ? a.v[5] : b.v[5],
        a.v[6] > b.v[6] ? a.v[6] : b.v[6], a.v[7] > b.v[7] ? a.v[7] : b.v[7]);
  }
  friend _i16x8 __pnacl_builtin_satadd(const _i16x8 a, const _i16x8 b) {
    return _i16x8(satadd_i16(a.v[0], b.v[0]), satadd_i16(a.v[1], b.v[1]),
                  satadd_i16(a.v[2], b.v[2]), satadd_i16(a.v[3], b.v[3]),
                  satadd_i16(a.v[4], b.v[4]), satadd_i16(a.v[5], b.v[5]),
                  satadd_i16(a.v[6], b.v[6]), satadd_i16(a.v[7], b.v[7]));
  }
  friend _i16x8 __pnacl_builtin_satsub(const _i16x8 a, const _i16x8 b) {
    return _i16x8(satsub_i16(a.v[0], b.v[0]), satsub_i16(a.v[1], b.v[1]),
                  satsub_i16(a.v[2], b.v[2]), satsub_i16(a.v[3], b.v[3]),
                  satsub_i16(a.v[4], b.v[4]), satsub_i16(a.v[5], b.v[5]),
                  satsub_i16(a.v[6], b.v[6]), satsub_i16(a.v[7], b.v[7]));
  }
  friend _i16x8 __pnacl_builtin_abs(const _i16x8 a) {
    return _i16x8(abs(a.v[0]), abs(a.v[1]), abs(a.v[2]), abs(a.v[3]),
                  abs(a.v[4]), abs(a.v[5]), abs(a.v[6]), abs(a.v[7]));
  }
  friend _i8x16 __pnacl_builtin_satpack(const _i16x8 a, const _i16x8 b) {
    return _i8x16(
        sat_i8(a.v[0]), sat_i8(a.v[1]), sat_i8(a.v[2]), sat_i8(a.v[3]),
        sat_i8(a.v[4]), sat_i8(a.v[5]), sat_i8(a.v[6]), sat_i8(a.v[7]),
        sat_i8(b.v[0]), sat_i8(b.v[1]), sat_i8(b.v[2]), sat_i8(b.v[3]),
        sat_i8(b.v[4]), sat_i8(b.v[5]), sat_i8(b.v[6]), sat_i8(b.v[7]));
  }
  friend _u8x16 __pnacl_builtin_usatpack(const _i16x8 a, const _i16x8 b) {
    return _u8x16(
        sat_u8(a.v[0]), sat_u8(a.v[1]), sat_u8(a.v[2]), sat_u8(a.v[3]),
        sat_u8(a.v[4]), sat_u8(a.v[5]), sat_u8(a.v[6]), sat_u8(a.v[7]),
        sat_u8(b.v[0]), sat_u8(b.v[1]), sat_u8(b.v[2]), sat_u8(b.v[3]),
        sat_u8(b.v[4]), sat_u8(b.v[5]), sat_u8(b.v[6]), sat_u8(b.v[7]));
  }

 private:
  static int16_t satadd_i16(const int16_t a, const int16_t b) {
    int32_t t = static_cast<int32_t>(a) + static_cast<int32_t>(b);
    if (t > SHRT_MAX) return SHRT_MAX;
    if (t < SHRT_MIN) return SHRT_MIN;
    return static_cast<int16_t>(t);
  }
  static int16_t satsub_i16(const int16_t a, const int16_t b) {
    int32_t t = static_cast<int32_t>(a) - static_cast<int32_t>(b);
    if (t > SHRT_MAX) return SHRT_MAX;
    if (t < SHRT_MIN) return SHRT_MIN;
    return static_cast<int16_t>(t);
  }
  static int8_t sat_i8(const int16_t a) {
    if (a > SCHAR_MAX) return SCHAR_MAX;
    if (a < SCHAR_MIN) return SCHAR_MIN;
    return static_cast<int8_t>(a);
  }
  static uint8_t sat_u8(const int16_t a) {
    if (a > UCHAR_MAX) return UCHAR_MAX;
    if (a < 0) return 0;
    return static_cast<uint8_t>(a);
  }

  union {
    int16_t v[8];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

class _u16x8 {
 public:
  _u16x8() { /* elements undefined */
  }
  _u16x8(uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t v4,
         uint16_t v5, uint16_t v6, uint16_t v7) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
    v[4] = v4;
    v[5] = v5;
    v[6] = v6;
    v[7] = v7;
  }
  explicit _u16x8(uint32_t b) {
    v[0] = v[1] = v[2] = v[3] = v[4] = v[5] = v[6] = v[7] = b;
  }
  explicit _u16x8(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  uint16_t& operator[](int i) { return v[i]; }
  const uint16_t& operator[](int i) const { return v[i]; }
  friend _u16x8 operator+(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3], a.v[4] + b.v[4], a.v[5] + b.v[5],
                  a.v[6] + b.v[6], a.v[7] + b.v[7]);
  }
  friend _u16x8 operator-(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3], a.v[4] - b.v[4], a.v[5] - b.v[5],
                  a.v[6] - b.v[6], a.v[7] - b.v[7]);
  }
  friend _u16x8 operator*(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3], a.v[4] * b.v[4], a.v[5] * b.v[5],
                  a.v[6] * b.v[6], a.v[7] * b.v[7]);
  }
#if defined(VREFOPTIONAL)
  friend _u16x8 operator/(const _u16x8 a, const _u16x8 b) {
    _u16x8 r;
    for (int i = 0; i < 8; i++) {
      if (b.v[i] == 0)
        r[i] = 0;
      else
        r[i] = a.v[i] / b.v[i];
    }
    return r;
  }
  friend _u16x8 operator%(const _u16x8 a, const _u16x8 b) {
    _u8x16 r;
    for (int i = 0; i < 8; i++) {
      if (b.v[i] == 0)
        r[i] = 0;
      else
        r[i] = a.v[i] % b.v[i];
    }
    return r;
  }
#endif
  friend _u16x8 operator<<(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] << b.v[0], a.v[1] << b.v[1], a.v[2] << b.v[2],
                  a.v[3] << b.v[3], a.v[4] << b.v[4], a.v[5] << b.v[5],
                  a.v[6] << b.v[6], a.v[7] << b.v[7]);
  }
  friend _u16x8 operator>>(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] >> b.v[0], a.v[1] >> b.v[1], a.v[2] >> b.v[2],
                  a.v[3] >> b.v[3], a.v[4] >> b.v[4], a.v[5] >> b.v[5],
                  a.v[6] >> b.v[6], a.v[7] >> b.v[7]);
  }
  friend _u16x8 operator~(const _u16x8 a) {
    return _u16x8(~a.v[0], ~a.v[1], ~a.v[2], ~a.v[3], ~a.v[4], ~a.v[5], ~a.v[6],
                  ~a.v[7]);
  }
  friend _u16x8 operator^(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] ^ b.v[0], a.v[1] ^ b.v[1], a.v[2] ^ b.v[2],
                  a.v[3] ^ b.v[3], a.v[4] ^ b.v[4], a.v[5] ^ b.v[5],
                  a.v[6] ^ b.v[6], a.v[7] ^ b.v[7]);
  }
  friend _u16x8 operator&(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] & b.v[0], a.v[1] & b.v[1], a.v[2] & b.v[2],
                  a.v[3] & b.v[3], a.v[4] & b.v[4], a.v[5] & b.v[5],
                  a.v[6] & b.v[6], a.v[7] & b.v[7]);
  }
  friend _u16x8 operator|(const _u16x8 a, const _u16x8 b) {
    return _u16x8(a.v[0] | b.v[0], a.v[1] | b.v[1], a.v[2] | b.v[2],
                  a.v[3] | b.v[3], a.v[4] | b.v[4], a.v[5] | b.v[5],
                  a.v[6] | b.v[6], a.v[7] | b.v[7]);
  }
  friend _i16x8 operator==(const _u16x8 a, const _u16x8 b) {
    return _i16x8(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0,
                  a.v[4] == b.v[4] ? -1 : 0, a.v[5] == b.v[5] ? -1 : 0,
                  a.v[6] == b.v[6] ? -1 : 0, a.v[7] == b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator!=(const _u16x8 a, const _u16x8 b) {
    return _i16x8(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0,
                  a.v[4] != b.v[4] ? -1 : 0, a.v[5] != b.v[5] ? -1 : 0,
                  a.v[6] != b.v[6] ? -1 : 0, a.v[7] != b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator<(const _u16x8 a, const _u16x8 b) {
    return _i16x8(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0,
                  a.v[4] < b.v[4] ? -1 : 0, a.v[5] < b.v[5] ? -1 : 0,
                  a.v[6] < b.v[6] ? -1 : 0, a.v[7] < b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator<=(const _u16x8 a, const _u16x8 b) {
    return _i16x8(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0,
                  a.v[4] <= b.v[4] ? -1 : 0, a.v[5] <= b.v[5] ? -1 : 0,
                  a.v[6] <= b.v[6] ? -1 : 0, a.v[7] <= b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator>(const _u16x8 a, const _u16x8 b) {
    return _i16x8(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0,
                  a.v[4] > b.v[4] ? -1 : 0, a.v[5] > b.v[5] ? -1 : 0,
                  a.v[6] > b.v[6] ? -1 : 0, a.v[7] > b.v[7] ? -1 : 0);
  }
  friend _i16x8 operator>=(const _u16x8 a, const _u16x8 b) {
    return _i16x8(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0,
                  a.v[4] >= b.v[4] ? -1 : 0, a.v[5] >= b.v[5] ? -1 : 0,
                  a.v[6] >= b.v[6] ? -1 : 0, a.v[7] >= b.v[7] ? -1 : 0);
  }
  friend _u16x8 __pnacl_builtin_shufflevector(const _u16x8 a, const _u16x8 b,
                                              const int i0, const int i1,
                                              const int i2, const int i3,
                                              const int i4, const int i5,
                                              const int i6, const int i7) {
    return _u16x8(
        i0 < 8 ? a.v[i0] : b.v[i0 - 8], i1 < 8 ? a.v[i1] : b.v[i1 - 8],
        i2 < 8 ? a.v[i2] : b.v[i2 - 8], i3 < 8 ? a.v[i3] : b.v[i3 - 8],
        i4 < 8 ? a.v[i4] : b.v[i4 - 8], i5 < 8 ? a.v[i5] : b.v[i5 - 8],
        i6 < 8 ? a.v[i6] : b.v[i6 - 8], i7 < 8 ? a.v[i7] : b.v[i7 - 8]);
  }
  friend _u16x8 __pnacl_builtin_load(_u16x8* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _u16x8 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _u16x8 __pnacl_builtin_min(const _u16x8 a, const _u16x8 b) {
    return _u16x8(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3],
        a.v[4] < b.v[4] ? a.v[4] : b.v[4], a.v[5] < b.v[5] ? a.v[5] : b.v[5],
        a.v[6] < b.v[6] ? a.v[6] : b.v[6], a.v[7] < b.v[7] ? a.v[7] : b.v[7]);
  }
  friend _u16x8 __pnacl_builtin_max(const _u16x8 a, const _u16x8 b) {
    return _u16x8(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3],
        a.v[4] > b.v[4] ? a.v[4] : b.v[4], a.v[5] > b.v[5] ? a.v[5] : b.v[5],
        a.v[6] > b.v[6] ? a.v[6] : b.v[6], a.v[7] > b.v[7] ? a.v[7] : b.v[7]);
  }
  friend _u16x8 __pnacl_builtin_satadd(const _u16x8 a, const _u16x8 b) {
    return _u16x8(satadd_u16(a.v[0], b.v[0]), satadd_u16(a.v[1], b.v[1]),
                  satadd_u16(a.v[2], b.v[2]), satadd_u16(a.v[3], b.v[3]),
                  satadd_u16(a.v[4], b.v[4]), satadd_u16(a.v[5], b.v[5]),
                  satadd_u16(a.v[6], b.v[6]), satadd_u16(a.v[7], b.v[7]));
  }
  friend _u16x8 __pnacl_builtin_satsub(const _u16x8 a, const _u16x8 b) {
    return _u16x8(satsub_u16(a.v[0], b.v[0]), satsub_u16(a.v[1], b.v[1]),
                  satsub_u16(a.v[2], b.v[2]), satsub_u16(a.v[3], b.v[3]),
                  satsub_u16(a.v[4], b.v[4]), satsub_u16(a.v[5], b.v[5]),
                  satsub_u16(a.v[6], b.v[6]), satsub_u16(a.v[7], b.v[7]));
  }
  friend _u8x16 __pnacl_builtin_usatpack(const _u16x8 a, const _u16x8 b) {
    return _u8x16(
        sat_u8(a.v[0]), sat_u8(a.v[1]), sat_u8(a.v[2]), sat_u8(a.v[3]),
        sat_u8(a.v[4]), sat_u8(a.v[5]), sat_u8(a.v[6]), sat_u8(a.v[7]),
        sat_u8(b.v[0]), sat_u8(b.v[1]), sat_u8(b.v[2]), sat_u8(b.v[3]),
        sat_u8(b.v[4]), sat_u8(b.v[5]), sat_u8(b.v[6]), sat_u8(b.v[7]));
  }

 private:
  static uint16_t satadd_u16(const uint16_t a, const uint16_t b) {
    int32_t t = static_cast<int32_t>(a) + static_cast<int32_t>(b);
    if (t < 0) return 0;
    if (t > USHRT_MAX) return USHRT_MAX;
    return static_cast<uint16_t>(t);
  }
  static uint16_t satsub_u16(const uint16_t a, const uint16_t b) {
    int32_t t = static_cast<int32_t>(a) - static_cast<int32_t>(b);
    if (t < 0) return 0;
    if (t > USHRT_MAX) return USHRT_MAX;
    return static_cast<uint16_t>(t);
  }
  static uint8_t sat_u8(const uint16_t a) {
    if (a > UCHAR_MAX) return UCHAR_MAX;
    return static_cast<uint8_t>(a);
  }

  union {
    uint16_t v[8];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

class _i32x4 {
 public:
  _i32x4() { /* elements undefined */
  }
  _i32x4(int32_t v0, int32_t v1, int32_t v2, int32_t v3) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }
  explicit _i32x4(int32_t b) { v[0] = v[1] = v[2] = v[3] = b; }
  explicit _i32x4(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  int32_t& operator[](int i) { return v[i]; }
  const int32_t& operator[](int i) const { return v[i]; }
  friend _i32x4 operator-(const _i32x4 a) {
    return _i32x4(-a.v[0], -a.v[1], -a.v[2], -a.v[3]);
  }
  friend _i32x4 operator+(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3]);
  }
  friend _i32x4 operator-(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3]);
  }
  friend _i32x4 operator*(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3]);
  }
#if defined(VREFOPTIONAL)
  friend _i32x4 operator/(const _i32x4 a, const _i32x4 b) {
    _i32x4 r;
    for (int i = 0; i < 4; i++) {
      if (b.v[i] == 0 || (a.v[i] == LONG_MIN && b.v[i] == -1))
        r[i] = 0;
      else
        r[i] = a.v[i] / b.v[i];
    }
    return r;
  }
  friend _i32x4 operator%(const _i32x4 a, const _i32x4 b) {
    _i32x4 r;
    for (int i = 0; i < 4; i++) {
      if (b.v[i] == 0 || (a.v[i] == LONG_MIN && b.v[i] == -1))
        r[i] = 0;
      else
        r[i] = a.v[i] % b.v[i];
    }
    return r;
  }
#endif
  friend _i32x4 operator<<(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] << b.v[0], a.v[1] << b.v[1], a.v[2] << b.v[2],
                  a.v[3] << b.v[3]);
  }
  friend _i32x4 operator>>(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] >> b.v[0], a.v[1] >> b.v[1], a.v[2] >> b.v[2],
                  a.v[3] >> b.v[3]);
  }
  friend _i32x4 operator~(const _i32x4 a) {
    return _i32x4(~a.v[0], ~a.v[1], ~a.v[2], ~a.v[3]);
  }
  friend _i32x4 operator^(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] ^ b.v[0], a.v[1] ^ b.v[1], a.v[2] ^ b.v[2],
                  a.v[3] ^ b.v[3]);
  }
  friend _i32x4 operator&(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] & b.v[0], a.v[1] & b.v[1], a.v[2] & b.v[2],
                  a.v[3] & b.v[3]);
  }
  friend _i32x4 operator|(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] | b.v[0], a.v[1] | b.v[1], a.v[2] | b.v[2],
                  a.v[3] | b.v[3]);
  }
  friend _i32x4 operator==(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator!=(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator>(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator>=(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator<(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator<=(const _i32x4 a, const _i32x4 b) {
    return _i32x4(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0);
  }
  friend _i32x4 __pnacl_builtin_shufflevector(const _i32x4 a, const _i32x4 b,
                                              const int i0, const int i1,
                                              const int i2, const int i3) {
    return _i32x4(
        i0 < 4 ? a.v[i0] : b.v[i0 - 4], i1 < 4 ? a.v[i1] : b.v[i1 - 4],
        i2 < 4 ? a.v[i2] : b.v[i2 - 4], i3 < 4 ? a.v[i3] : b.v[i3 - 4]);
  }
  friend _i32x4 __pnacl_builtin_load(_i32x4* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _i32x4 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _i32x4 __pnacl_builtin_min(const _i32x4 a, const _i32x4 b) {
    return _i32x4(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3]);
  }
  friend _i32x4 __pnacl_builtin_max(const _i32x4 a, const _i32x4 b) {
    return _i32x4(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3]);
  }
  friend _i32x4 __pnacl_builtin_satadd(const _i32x4 a, const _i32x4 b) {
    return _i32x4(satadd_i32(a.v[0], b.v[0]), satadd_i32(a.v[1], b.v[1]),
                  satadd_i32(a.v[2], b.v[2]), satadd_i32(a.v[3], b.v[3]));
  }
  friend _i32x4 __pnacl_builtin_satsub(const _i32x4 a, const _i32x4 b) {
    return _i32x4(satsub_i32(a.v[0], b.v[0]), satsub_i32(a.v[1], b.v[1]),
                  satsub_i32(a.v[2], b.v[2]), satsub_i32(a.v[3], b.v[3]));
  }
  friend _i32x4 __pnacl_builtin_abs(const _i32x4 a) {
    return _i32x4(abs(a.v[0]), abs(a.v[1]), abs(a.v[2]), abs(a.v[3]));
  }
  friend _i16x8 __pnacl_builtin_satpack(const _i32x4 a, const _i32x4 b) {
    return _i16x8(sat_i16(a.v[0]), sat_i16(a.v[1]), sat_i16(a.v[2]),
                  sat_i16(a.v[3]), sat_i16(b.v[0]), sat_i16(b.v[1]),
                  sat_i16(b.v[2]), sat_i16(b.v[3]));
  }
  friend _u16x8 __pnacl_builtin_usatpack(const _i32x4 a, const _i32x4 b) {
    return _u16x8(sat_u16(a.v[0]), sat_u16(a.v[1]), sat_u16(a.v[2]),
                  sat_u16(a.v[3]), sat_u16(b.v[0]), sat_u16(b.v[1]),
                  sat_u16(b.v[2]), sat_u16(b.v[3]));
  }

 private:
  static int32_t satadd_i32(const int32_t a, const int32_t b) {
    int64_t t = static_cast<int64_t>(a) + static_cast<int64_t>(b);
    if (t > INT_MAX) return INT_MAX;
    if (t < INT_MIN) return INT_MIN;
    return static_cast<int32_t>(t);
  }
  static int32_t satsub_i32(const int32_t a, const int32_t b) {
    int64_t t = static_cast<int64_t>(a) - static_cast<int64_t>(b);
    if (t > INT_MAX) return INT_MAX;
    if (t < INT_MIN) return INT_MIN;
    return static_cast<int32_t>(t);
  }
  static int16_t sat_i16(const int32_t a) {
    if (a > SHRT_MAX) return SHRT_MAX;
    if (a < SHRT_MIN) return SHRT_MIN;
    return static_cast<int16_t>(a);
  }
  static uint16_t sat_u16(const int32_t a) {
    if (a > USHRT_MAX) return USHRT_MAX;
    if (a < 0) return 0;
    return static_cast<uint16_t>(a);
  }

  union {
    int32_t v[4];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

class _u32x4 {
 public:
  _u32x4() { /* elements undefined */
  }
  _u32x4(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }
  explicit _u32x4(uint32_t b) { v[0] = v[1] = v[2] = v[3] = b; }
  explicit _u32x4(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  uint32_t& operator[](int i) { return v[i]; }
  const uint32_t& operator[](int i) const { return v[i]; }
  friend _u32x4 operator-(const _u32x4 a) {
    return _u32x4(-a.v[0], -a.v[1], -a.v[2], -a.v[3]);
  }
  friend _u32x4 operator+(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3]);
  }
  friend _u32x4 operator-(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3]);
  }
  friend _u32x4 operator*(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3]);
  }
#if defined(VREFOPTIONAL)
  friend _u32x4 operator/(const _u32x4 a, const _u32x4 b) {
    _u32x4 r;
    for (int i = 0; i < 4; i++) {
      if (b.v[i] == 0)
        r[i] = 0;
      else
        r[i] = a.v[i] / b.v[i];
    }
    return r;
  }
  friend _u32x4 operator%(const _u32x4 a, const _u32x4 b) {
    _u32x4 r;
    for (int i = 0; i < 4; i++) {
      if (b.v[i] == 0)
        r[i] = 0;
      else
        r[i] = a.v[i] % b.v[i];
    }
    return r;
  }
#endif
  friend _u32x4 operator<<(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] << b.v[0], a.v[1] << b.v[1], a.v[2] << b.v[2],
                  a.v[3] << b.v[3]);
  }
  friend _u32x4 operator>>(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] >> b.v[0], a.v[1] >> b.v[1], a.v[2] >> b.v[2],
                  a.v[3] >> b.v[3]);
  }
  friend _u32x4 operator~(const _u32x4 a) {
    return _u32x4(~a.v[0], ~a.v[1], ~a.v[2], ~a.v[3]);
  }
  friend _u32x4 operator^(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] ^ b.v[0], a.v[1] ^ b.v[1], a.v[2] ^ b.v[2],
                  a.v[3] ^ b.v[3]);
  }
  friend _u32x4 operator&(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] & b.v[0], a.v[1] & b.v[1], a.v[2] & b.v[2],
                  a.v[3] & b.v[3]);
  }
  friend _u32x4 operator|(const _u32x4 a, const _u32x4 b) {
    return _u32x4(a.v[0] | b.v[0], a.v[1] | b.v[1], a.v[2] | b.v[2],
                  a.v[3] | b.v[3]);
  }
  friend _i32x4 operator==(const _u32x4 a, const _u32x4 b) {
    return _i32x4(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator!=(const _u32x4 a, const _u32x4 b) {
    return _i32x4(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator>(const _u32x4 a, const _u32x4 b) {
    return _i32x4(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator>=(const _u32x4 a, const _u32x4 b) {
    return _i32x4(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator<(const _u32x4 a, const _u32x4 b) {
    return _i32x4(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator<=(const _u32x4 a, const _u32x4 b) {
    return _i32x4(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0);
  }
  friend _u32x4 __pnacl_builtin_shufflevector(const _u32x4 a, const _u32x4 b,
                                              const int i0, const int i1,
                                              const int i2, const int i3) {
    return _u32x4(
        i0 < 4 ? a.v[i0] : b.v[i0 - 4], i1 < 4 ? a.v[i1] : b.v[i1 - 4],
        i2 < 4 ? a.v[i2] : b.v[i2 - 4], i3 < 4 ? a.v[i3] : b.v[i3 - 4]);
  }
  friend _u32x4 __pnacl_builtin_load(_u32x4* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _u32x4 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _u32x4 __pnacl_builtin_min(const _u32x4 a, const _u32x4 b) {
    return _u32x4(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3]);
  }
  friend _u32x4 __pnacl_builtin_max(const _u32x4 a, const _u32x4 b) {
    return _u32x4(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3]);
  }
  friend _u32x4 __pnacl_builtin_satadd(const _u32x4 a, const _u32x4 b) {
    return _u32x4(satadd_u32(a.v[0], b.v[0]), satadd_u32(a.v[1], b.v[1]),
                  satadd_u32(a.v[2], b.v[2]), satadd_u32(a.v[3], b.v[3]));
  }
  friend _u32x4 __pnacl_builtin_satsub(const _u32x4 a, const _u32x4 b) {
    return _u32x4(satsub_u32(a.v[0], b.v[0]), satsub_u32(a.v[1], b.v[1]),
                  satsub_u32(a.v[2], b.v[2]), satsub_u32(a.v[3], b.v[3]));
  }
  friend _u16x8 __pnacl_builtin_usatpack(const _u32x4 a, const _u32x4 b) {
    return _u16x8(sat_u16(a.v[0]), sat_u16(a.v[1]), sat_u16(a.v[2]),
                  sat_u16(a.v[3]), sat_u16(b.v[0]), sat_u16(b.v[1]),
                  sat_u16(b.v[2]), sat_u16(b.v[3]));
  }

 private:
  static uint32_t satadd_u32(const uint32_t a, const uint32_t b) {
    uint64_t t = static_cast<uint64_t>(a) + static_cast<uint64_t>(b);
    if (t > UINT_MAX) return UINT_MAX;
    return static_cast<uint32_t>(t);
  }
  static uint32_t satsub_u32(const uint32_t a, const uint32_t b) {
    uint64_t t = static_cast<uint64_t>(a) - static_cast<uint64_t>(b);
    if (t > UINT_MAX) return UINT_MAX;
    return static_cast<uint32_t>(t);
  }
  static uint16_t sat_u16(const uint32_t a) {
    if (a > USHRT_MAX) return USHRT_MAX;
    return static_cast<uint16_t>(a);
  }

  union {
    uint32_t v[4];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

class _f32x4 {
 public:
  _f32x4() { /* elements undefined */
  }
  _f32x4(float v0, float v1, float v2, float v3) {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }
  explicit _f32x4(float b) { v[0] = v[1] = v[2] = v[3] = b; }
  explicit _f32x4(_x64x2 u) { x64x2 = u; }
  operator _x64x2() { return x64x2; }
  float& operator[](int i) { return v[i]; }
  const float& operator[](int i) const { return v[i]; }
  friend _f32x4 operator-(const _f32x4 a) {
    return _f32x4(-a.v[0], -a.v[1], -a.v[2], -a.v[3]);
  }
  friend _f32x4 operator+(const _f32x4 a, const _f32x4 b) {
    return _f32x4(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2],
                  a.v[3] + b.v[3]);
  }
  friend _f32x4 operator-(const _f32x4 a, const _f32x4 b) {
    return _f32x4(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2],
                  a.v[3] - b.v[3]);
  }
  friend _f32x4 operator*(const _f32x4 a, const _f32x4 b) {
    return _f32x4(a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2],
                  a.v[3] * b.v[3]);
  }
  friend _f32x4 operator/(const _f32x4 a, const _f32x4 b) {
    return _f32x4(a.v[0] / b.v[0], a.v[1] / b.v[1], a.v[2] / b.v[2],
                  a.v[3] / b.v[3]);
  }
  friend _i32x4 operator==(const _f32x4 a, const _f32x4 b) {
    return _i32x4(a.v[0] == b.v[0] ? -1 : 0, a.v[1] == b.v[1] ? -1 : 0,
                  a.v[2] == b.v[2] ? -1 : 0, a.v[3] == b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator!=(const _f32x4 a, const _f32x4 b) {
    return _i32x4(a.v[0] != b.v[0] ? -1 : 0, a.v[1] != b.v[1] ? -1 : 0,
                  a.v[2] != b.v[2] ? -1 : 0, a.v[3] != b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator>(const _f32x4 a, const _f32x4 b) {
    return _i32x4(a.v[0] > b.v[0] ? -1 : 0, a.v[1] > b.v[1] ? -1 : 0,
                  a.v[2] > b.v[2] ? -1 : 0, a.v[3] > b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator>=(const _f32x4 a, const _f32x4 b) {
    return _i32x4(a.v[0] >= b.v[0] ? -1 : 0, a.v[1] >= b.v[1] ? -1 : 0,
                  a.v[2] >= b.v[2] ? -1 : 0, a.v[3] >= b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator<(const _f32x4 a, const _f32x4 b) {
    return _i32x4(a.v[0] < b.v[0] ? -1 : 0, a.v[1] < b.v[1] ? -1 : 0,
                  a.v[2] < b.v[2] ? -1 : 0, a.v[3] < b.v[3] ? -1 : 0);
  }
  friend _i32x4 operator<=(const _f32x4 a, const _f32x4 b) {
    return _i32x4(a.v[0] <= b.v[0] ? -1 : 0, a.v[1] <= b.v[1] ? -1 : 0,
                  a.v[2] <= b.v[2] ? -1 : 0, a.v[3] <= b.v[3] ? -1 : 0);
  }
  friend _f32x4 __pnacl_builtin_shufflevector(const _f32x4 a, const _f32x4 b,
                                              const int i0, const int i1,
                                              const int i2, const int i3) {
    return _f32x4(
        i0 < 4 ? a.v[i0] : b.v[i0 - 4], i1 < 4 ? a.v[i1] : b.v[i1 - 4],
        i2 < 4 ? a.v[i2] : b.v[i2 - 4], i3 < 4 ? a.v[i3] : b.v[i3 - 4]);
  }
  friend _f32x4 __pnacl_builtin_load(_f32x4* v, const void* addr) {
    memcpy(v, addr, sizeof(*v));
    return *v;
  }
  friend void __pnacl_builtin_store(void* addr, const _f32x4 v) {
    memcpy(addr, &v, sizeof(v));
  }
  friend _f32x4 __pnacl_builtin_min(const _f32x4 a, const _f32x4 b) {
    return _f32x4(
        a.v[0] < b.v[0] ? a.v[0] : b.v[0], a.v[1] < b.v[1] ? a.v[1] : b.v[1],
        a.v[2] < b.v[2] ? a.v[2] : b.v[2], a.v[3] < b.v[3] ? a.v[3] : b.v[3]);
  }
  friend _f32x4 __pnacl_builtin_max(const _f32x4 a, const _f32x4 b) {
    return _f32x4(
        a.v[0] > b.v[0] ? a.v[0] : b.v[0], a.v[1] > b.v[1] ? a.v[1] : b.v[1],
        a.v[2] > b.v[2] ? a.v[2] : b.v[2], a.v[3] > b.v[3] ? a.v[3] : b.v[3]);
  }
  friend _f32x4 __pnacl_builtin_abs(const _f32x4 a) {
    return _f32x4(fabs(a.v[0]), fabs(a.v[1]), fabs(a.v[2]), fabs(a.v[3]));
  }
  friend _f32x4 __pnacl_builtin_reciprocal(const _f32x4 a) {
    return _f32x4(1.0f / a.v[0], 1.0f / a.v[1], 1.0f / a.v[2], 1.0f / a.v[3]);
  }
  friend _f32x4 __pnacl_builtin_reciprocalsqrt(const _f32x4 a) {
    return _f32x4(1.0f / sqrtf(a.v[0]), 1.0f / sqrtf(a.v[1]),
                  1.0f / sqrtf(a.v[2]), 1.0f / sqrtf(a.v[3]));
  }
  friend _f32x4 __pnacl_builtin_sqrt(const _f32x4 a) {
    return _f32x4(sqrtf(a.v[0]), sqrtf(a.v[1]), sqrtf(a.v[2]), sqrtf(a.v[3]));
  }
  friend _f32x4 __pnacl_builtin_convertvector(_f32x4* r, const _i32x4 a) {
    return *r = _f32x4(static_cast<float>(a[0]), static_cast<float>(a[1]),
                       static_cast<float>(a[2]), static_cast<float>(a[3]));
  }
  friend _f32x4 __pnacl_builtin_convertvector(_f32x4* r, const _u32x4 a) {
    return *r = _f32x4(static_cast<float>(a[0]), static_cast<float>(a[1]),
                       static_cast<float>(a[2]), static_cast<float>(a[3]));
  }
  friend _i32x4 __pnacl_builtin_convertvector(_i32x4* r, const _f32x4 a) {
    return *r = _i32x4(static_cast<int32_t>(a[0]), static_cast<int32_t>(a[1]),
                       static_cast<int32_t>(a[2]), static_cast<int32_t>(a[3]));
  }
  friend _u32x4 __pnacl_builtin_convertvector(_u32x4* r, const _f32x4 a) {
    return *r =
               _u32x4(static_cast<uint32_t>(a[0]), static_cast<uint32_t>(a[1]),
                      static_cast<uint32_t>(a[2]), static_cast<uint32_t>(a[3]));
  }
  friend int32_t __pnacl_builtin_signbits(const _f32x4 a) {
    return ((a.v[0] < 0.0f ? 1 : 0) | (a.v[1] < 0.0f ? 2 : 0) |
            (a.v[2] < 0.0f ? 4 : 0) | (a.v[3] < 0.0f ? 8 : 0));
  }

 private:
  union {
    float v[4];
    _x64x2 x64x2;
  };
} __attribute__((aligned(16)));

#else

// Implemented via builtin vector types and intrinsics.

typedef int8_t _i8x16 __attribute__((vector_size(16)));
typedef uint8_t _u8x16 __attribute__((vector_size(16)));
typedef int16_t _i16x8 __attribute__((vector_size(16)));
typedef uint16_t _u16x8 __attribute__((vector_size(16)));
typedef int32_t _i32x4 __attribute__((vector_size(16)));
typedef uint32_t _u32x4 __attribute__((vector_size(16)));
typedef float _f32x4 __attribute__((vector_size(16)));

#if !defined(__clang__)
ALWAYS_INLINE
_f32x4 __pnacl_builtin_shufflevector(_f32x4 a, _f32x4 b, const int i0,
                                     const int i1, const int i2, const int i3) {
  const _u32x4 index_vec = {i0, i1, i2, i3};
  return __builtin_shuffle(a, b, index_vec);
}

ALWAYS_INLINE
_u32x4 __pnacl_builtin_shufflevector(_u32x4 a, _u32x4 b, const int i0,
                                     const int i1, const int i2, const int i3) {
  const _u32x4 index_vec = {i0, i1, i2, i3};
  return __builtin_shuffle(a, b, index_vec);
}

ALWAYS_INLINE
_i32x4 __pnacl_builtin_shufflevector(_i32x4 a, _i32x4 b, const int i0,
                                     const int i1, const int i2, const int i3) {
  const _i32x4 index_vec = {i0, i1, i2, i3};
  return __builtin_shuffle(a, b, index_vec);
}

ALWAYS_INLINE
_u16x8 __pnacl_builtin_shufflevector(_u16x8 a, _u16x8 b, const int i0,
                                     const int i1, const int i2, const int i3,
                                     const int i4, const int i5, const int i6,
                                     const int i7) {
  const _u16x8 index_vec = {i0, i1, i2, i3, i4, i5, i6, i7};
  return __builtin_shuffle(a, b, index_vec);
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_shufflevector(_i16x8 a, _i16x8 b, const int i0,
                                     const int i1, const int i2, const int i3,
                                     const int i4, const int i5, const int i6,
                                     const int i7) {
  const _i16x8 index_vec = {i0, i1, i2, i3, i4, i5, i6, i7};
  return __builtin_shuffle(a, b, index_vec);
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_shufflevector(_u8x16 a, _u8x16 b, const int i0,
                                     const int i1, const int i2, const int i3,
                                     const int i4, const int i5, const int i6,
                                     const int i7, const int i8, const int i9,
                                     const int i10, const int i11,
                                     const int i12, const int i13,
                                     const int i14, const int i15) {
  const _u8x16 index_vec = {i0, i1, i2,  i3,  i4,  i5,  i6,  i7,
                            i8, i9, i10, i11, i12, i13, i14, i15};
  return __builtin_shuffle(a, b, index_vec);
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_shufflevector(_i8x16 a, _i8x16 b, const int i0,
                                     const int i1, const int i2, const int i3,
                                     const int i4, const int i5, const int i6,
                                     const int i7, const int i8, const int i9,
                                     const int i10, const int i11,
                                     const int i12, const int i13,
                                     const int i14, const int i15) {
  const _i8x16 index_vec = {i0, i1, i2,  i3,  i4,  i5,  i6,  i7,
                            i8, i9, i10, i11, i12, i13, i14, i15};
  return __builtin_shuffle(a, b, index_vec);
}

#elif !defined(__arm__)

#define __pnacl_builtin_shufflevector __builtin_shufflevector

#else

// A slightly faster version for byte sized shuffles on arm using vtbl opcode.
ALWAYS_INLINE
_u8x16 __pnacl_builtin_shufflevector_u8x16(
    const _u8x16 a, const _u8x16 b, const uint8_t i0, const uint8_t i1,
    const uint8_t i2, const uint8_t i3, const uint8_t i4, const uint8_t i5,
    const uint8_t i6, const uint8_t i7, const uint8_t i8, const uint8_t i9,
    const uint8_t i10, const uint8_t i11, const uint8_t i12, const uint8_t i13,
    const uint8_t i14, const uint8_t i15) {
  const _u8x16 ab[2] = {a, b};
  const uint8x8x4_t a0 = *(uint8x8x4_t*)ab;
  const uint8x8_t d0 = {i0, i1, i2, i3, i4, i5, i6, i7};
  const uint8x8_t d1 = {i8, i9, i10, i11, i12, i13, i14, i15};
  uint8x8x2_t r;
  r.val[0] = vtbl4_u8(a0, d0);
  r.val[1] = vtbl4_u8(a0, d1);
  return *(_u8x16*)&r;
}

#define __pnacl_builtin_shufflevector6(a, b, i0, i1, i2, i3) \
  __builtin_shufflevector((a), (b), (i0), (i1), (i2), (i3))
#define __pnacl_builtin_shufflevector10(a, b, i0, i1, i2, i3, i4, i5, i6, i7) \
  __builtin_shufflevector((a), (b), (i0), (i1), (i2), (i3), (i4), (i5), (i6), \
                          (i7))
#define __pnacl_builtin_shufflevector18(a, b, i0, i1, i2, i3, i4, i5, i6, i7, \
                                        i8, i9, i10, i11, i12, i13, i14, i15) \
  __pnacl_builtin_shufflevector_u8x16((a), (b), (i0), (i1), (i2), (i3), (i4), \
                                      (i5), (i6), (i7), (i8), (i9), (i10),    \
                                      (i11), (i12), (i13), (i14), (i15))

// Use macro below to select appropriate macro from above depending on
// number of arguments.  Macros above can then select either passing to
// __builtin_shufflevector or in the case of byte sized shuffles on arm,
// invoke a special inline version which uses vtbl.  In pnacl, we can
// override __builtin_shufflevector implementation in the back end,
// instead of here.

#define __GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, \
                    _14, _15, _16, _i17, _18, _19, _20, NAME, ...)          \
  NAME
#define __pnacl_builtin_shufflevector(...)                              \
  __GET_MACRO(                                                          \
      __VA_ARGS__, __pnacl_builtin_shufflevector20,                     \
      __pnacl_builtin_shufflevector19, __pnacl_builtin_shufflevector18, \
      __pnacl_builtin_shufflevector17, __pnacl_builtin_shufflevector16, \
      __pnacl_builtin_shufflevector15, __pnacl_builtin_shufflevector14, \
      __pnacl_builtin_shufflevector13, __pnacl_builtin_shufflevector12, \
      __pnacl_builtin_shufflevector11, __pnacl_builtin_shufflevector10, \
      __pnacl_builtin_shufflevector9, __pnacl_builtin_shufflevector8,   \
      __pnacl_builtin_shufflevector7, __pnacl_builtin_shufflevector6,   \
      __pnacl_builtin_shufflevector5, __pnacl_builtin_shufflevector4,   \
      __pnacl_builtin_shufflevector3, __pnacl_builtin_shufflevector2,   \
      __pnacl_builtin_shufflevector1)(__VA_ARGS__)

#endif

ALWAYS_INLINE
_i8x16 __pnacl_builtin_load(_i8x16* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_s8((const int8_t*)addr); /* NEON */
#elif defined(__x86__)
  return *a = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _i8x16 a) {
#if defined(__arm__)
  vst1q_s8((int8_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, a);               /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_min(const _i8x16 a, const _i8x16 b) {
#if defined(__arm__)
  return vminq_s8(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_min_epi8(a, b);                         /* SSE4 */
#else
  _u8x16 m = a < b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_max(const _i8x16 a, const _i8x16 b) {
#if defined(__arm__)
  return vmaxq_s8(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_max_epi8(a, b);                         /* SSE4 */
#else
  _u8x16 m = a > b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_satadd(const _i8x16 a, const _i8x16 b) {
#if defined(__arm__)
  return vqaddq_s8(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_adds_epi8(a, b);                        /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_satsub(const _i8x16 a, const _i8x16 b) {
#if defined(__arm__)
  return vqsubq_s8(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_subs_epi8(a, b);                        /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_abs(const _i8x16 a) {
#if defined(__arm__)
  return vabsq_s8(a); /* NEON */
#elif defined(__x86__)
#if defined(__SSSE3__) || defined(__SSE4__)
  return _mm_abs_epi8(a);                            /* SSSE3 */
#else
  _i8x16 z = _i8x16(0);
  _i8x16 positive = __pnacl_builtin_max(a, z);
  _i8x16 negative = __pnacl_builtin_min(a, z);
  return positive - negative;
#endif
#else
#error "Reached unsupported configuration"
#endif
}

// _u8x16 builtins

ALWAYS_INLINE
_u8x16 __pnacl_builtin_load(_u8x16* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_u8((const uint8_t*)addr); /* NEON */
#elif defined(__x86__)
  return *a = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _u8x16 a) {
#if defined(__arm__)
  vst1q_u8((uint8_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, a);               /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_min(const _u8x16 a, const _u8x16 b) {
#if defined(__arm__)
  return vminq_u8(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_min_epu8(a, b);                         /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_max(const _u8x16 a, const _u8x16 b) {
#if defined(__arm__)
  return vmaxq_u8(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_max_epu8(a, b);                         /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_satadd(const _u8x16 a, const _u8x16 b) {
#if defined(__arm__)
  return vqaddq_u8(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_adds_epu8(a, b);                        /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_satsub(const _u8x16 a, const _u8x16 b) {
#if defined(__arm__)
  return vqsubq_u8(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_subs_epu8(a, b);                        /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_avg(const _u8x16 a, const _u8x16 b) {
#if defined(__arm__)
  /* TODO: better emu for NEON, below is obviously wrong and will overflow */
  _u8x16 c = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  return (a + b) >> c;
#elif defined(__x86__)
  return _mm_avg_epu8(a, b);                         /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

// _i16x8

ALWAYS_INLINE
_i16x8 __pnacl_builtin_load(_i16x8* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_s16((const int16_t*)addr); /* NEON */
#elif defined(__x86__)
  return *a = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _i16x8 a) {
#if defined(__arm__)
  vst1q_s16((int16_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, a);               /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_min(const _i16x8 a, const _i16x8 b) {
#if defined(__arm__)
  return vminq_s16(a, b);
#elif defined(__x86__)
  return _mm_min_epi16(a, b);                        /* SSE2 */
#else
#error "Reached upsupported configuration"
#endif
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_max(const _i16x8 a, const _i16x8 b) {
#if defined(__arm__)
  return vmaxq_s16(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_max_epi16(a, b);                        /* SSE2 */
#else
#error "Reached upsupported configuration"
#endif
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_satadd(const _i16x8 a, const _i16x8 b) {
#if defined(__arm__)
  return vqaddq_s16(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_adds_epi16(a, b);                       /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_satsub(const _i16x8 a, const _i16x8 b) {
#if defined(__arm__)
  return vqsubq_s16(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_subs_epi16(a, b);                       /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_abs(const _i16x8 a) {
#if defined(__arm__)
  return vabsq_s16(a); /* NEON */
#elif defined(__x86__)
#if defined(__SSSE3__) || defined(__SSE4__)
  return _mm_abs_epi16(a);                           /* SSSE3 */
#else
  _i16x8 zero = _i16x8(0);
  _i16x8 positive = __pnacl_builtin_max(a, zero);
  _i16x8 negative = __pnacl_builtin_min(a, zero);
  return positive - negative;
#endif
#else
#error "Reached upsupported configuration"
#endif
}

// _u16x8

ALWAYS_INLINE
_u16x8 __pnacl_builtin_load(_u16x8* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_u16((const uint16_t*)addr); /* NEON */
#elif defined(__x86__)
  return *a = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _u16x8 a) {
#if defined(__arm__)
  vst1q_u16((uint16_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, a);               /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u16x8 __pnacl_builtin_max(const _u16x8 a, const _u16x8 b) {
#if defined(__arm__)
  return vmaxq_u16(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_max_epu16(a, b);                        /* SSE4 */
#else
  _u16x8 m = a > b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u16x8 __pnacl_builtin_min(const _u16x8 a, const _u16x8 b) {
#if defined(__arm__)
  return vminq_u16(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_min_epu16(a, b);                        /* SSE4 */
#else
  _u16x8 m = a < b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u16x8 __pnacl_builtin_satadd(const _u16x8 a, const _u16x8 b) {
#if defined(__arm__)
  return vqaddq_u16(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_adds_epu16(a, b);                       /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u16x8 __pnacl_builtin_satsub(const _u16x8 a, const _u16x8 b) {
#if defined(__arm__)
  return vqsubq_u16(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_subs_epu16(a, b);                       /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

// _i32x4

ALWAYS_INLINE
_i32x4 __pnacl_builtin_load(_i32x4* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_s32((const int32_t*)addr); /* NEON */
#elif defined(__x86__)
  return *a = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _i32x4 a) {
#if defined(__arm__)
  vst1q_s32((int32_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, (__m128i)a);      /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i32x4 __pnacl_builtin_min(const _i32x4 a, const _i32x4 b) {
#if defined(__arm__)
  return vminq_s32(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_min_epi32(a, b);                        /* SSE4 */
#else
  _i32x4 m = a < b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i32x4 __pnacl_builtin_max(const _i32x4 a, const _i32x4 b) {
#if defined(__arm__)
  return vmaxq_s32(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_max_epi32(a, b);                        /* SSE4 */
#else
  _i32x4 m = a > b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

#if defined(__x86__)
ALWAYS_INLINE
int32_t satadd_i32(int32_t a, int32_t b) {
  int64_t t = (int64_t)a + (int64_t)b;
  if (t > LONG_MAX) return static_cast<int32_t>(LONG_MAX);
  if (t < LONG_MIN) return static_cast<int32_t>(LONG_MIN);
  return (int32_t)t;
}
#endif

ALWAYS_INLINE
_i32x4 __pnacl_builtin_satadd(const _i32x4 a, const _i32x4 b) {
#if defined(__arm__)
  return vqaddq_s32(a, b); /* NEON */
#elif defined(__x86__)
  _i32x4 t = {satadd_i32(a[0], b[0]), satadd_i32(a[1], b[1]),
              satadd_i32(a[2], b[2]), satadd_i32(a[3], b[3])};
  return t;
#else
#error "Reached unsupported configuration"
#endif
}

#if defined(__x86__)
ALWAYS_INLINE
int32_t satsub_i32(int32_t a, int32_t b) {
  int64_t t = (int64_t)a - (int64_t)b;
  if (t > LONG_MAX) return static_cast<int32_t>(LONG_MAX);
  if (t < LONG_MIN) return static_cast<int32_t>(LONG_MIN);
  return (int32_t)t;
}
#endif

ALWAYS_INLINE
_i32x4 __pnacl_builtin_satsub(const _i32x4 a, const _i32x4 b) {
#if defined(__arm__)
  return vqsubq_s32(a, b); /* NEON */
#elif defined(__x86__)
  _i32x4 t = {satsub_i32(a[0], b[0]), satsub_i32(a[1], b[1]),
              satsub_i32(a[2], b[2]), satsub_i32(a[3], b[3])};
  return t;
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i32x4 __pnacl_builtin_abs(const _i32x4 a) {
#if defined(__arm__)
  return vabsq_s32(a);
#elif defined(__x86__)
#if defined(__SSSE3__) || defined(__SSE4__)
  return _mm_abs_epi32(a);                           /* SSSE3 */
#else
  /* fallback version */
  _i32x4 z = _i32x4(0);
  _i32x4 positive = __pnacl_builtin_max(a, z);
  _i32x4 negative = __pnacl_builtin_min(a, z);
  return positive - negative;
#endif
#else
#error "Reached unsupported configuration"
#endif
}

// _u32x4

ALWAYS_INLINE
_u32x4 __pnacl_builtin_load(_u32x4* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_u32((const uint32_t*)addr); /* NEON */
#elif defined(__x86__)
  return *a = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _u32x4 a) {
#if defined(__arm__)
  vst1q_u32((uint32_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, a);               /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u32x4 __pnacl_builtin_min(const _u32x4 a, const _u32x4 b) {
#if defined(__arm__)
  return vminq_u32(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_min_epu32(a, b);                        /* SSE4 */
#else
  _u32x4 m = a < b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u32x4 __pnacl_builtin_max(const _u32x4 a, const _u32x4 b) {
#if defined(__arm__)
  return vmaxq_u32(a, b); /* NEON */
#elif defined(__x86__)
#if defined(__SSE4__)
  return _mm_max_epu32(a, b);                        /* SSE4 */
#else
  _u32x4 m = a > b;
  return (m & a) | (~m & b);
#endif
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u32x4 __pnacl_builtin_satadd(const _u32x4 a, const _u32x4 b) {
#if defined(__arm__)
  return vqaddq_u32(a, b); /* NEON */
#else
  /* 32 bit unsigned saturated add NOT AVAILABLE IN SSE */
  _u32x4 sum = a + b;
  _u32x4 ca = sum < a;
  _u32x4 cb = sum < b;
  return sum | ca | cb;
#endif
}

ALWAYS_INLINE
_u32x4 __pnacl_builtin_satsub(const _u32x4 a, const _u32x4 b) {
#if defined(__arm__)
  return vqsubq_u32(a, b); /* NEON */
#else
  /* 32 bit unsigned saturated sub NOT AVAILABLE IN SSE */
  _u32x4 sum = a - b;
  _u32x4 ca = sum > a;
  _u32x4 cb = sum > b;
  return sum | ca | cb;
#endif
}

// _f32x4

ALWAYS_INLINE
_f32x4 __pnacl_builtin_load(_f32x4* a, const void* addr) {
#if defined(__arm__)
  return *a = vld1q_f32((float32_t*)addr); /* NEON */
#elif defined(__x86__)
  __m128i t = _mm_loadu_si128((const __m128i*)addr); /* SSE2 */
  return *a = *(_f32x4*)&t;
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
void __pnacl_builtin_store(void* addr, const _f32x4 a) {
#if defined(__arm__)
  vst1q_f32((float32_t*)addr, a); /* NEON */
#elif defined(__x86__)
  _mm_storeu_si128((__m128i*)addr, *(__m128i*)&a); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_min(const _f32x4 a, const _f32x4 b) {
#if defined(__arm__)
  return vminq_f32(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_min_ps(a, b);                         /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_max(const _f32x4 a, const _f32x4 b) {
#if defined(__arm__)
  return vmaxq_f32(a, b); /* NEON */
#elif defined(__x86__)
  return _mm_max_ps(a, b);                         /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_abs(const _f32x4 a) {
  _u32x4 temp = (_u32x4)a;
  _u32x4 p = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
  temp = temp & p;
  return (_f32x4)temp;
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_reciprocalsqrt(_f32x4 a) {
#if defined(__arm__)
  _f32x4 x = vrsqrteq_f32(a); /* NEON */
#elif defined(__x86__)
  _f32x4 x = _mm_rsqrt_ps(a);                      /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
  _f32x4 p = {1.5f, 1.5f, 1.5f, 1.5f};
  _f32x4 q = {0.5f, 0.5f, 0.5f, 0.5f};
  return x * (p - q * a * x * x);
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_sqrt(_f32x4 a) {
#if defined(__arm__)
  _f32x4 one = {1.0f, 1.0f, 1.0f, 1.0f};
  return one / vrsqrteq_f32(a); /* NEON */
#elif defined(__x86__)
  return _mm_sqrt_ps(a);                           /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_convertvector(_f32x4* r, const _i32x4 a) {
#if defined(__arm__)
  return *r = vcvtq_f32_s32(a); /* NEON */
#elif defined(__x86__)
  return *r = _mm_cvtepi32_ps(a);                  /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_f32x4 __pnacl_builtin_convertvector(_f32x4* r, const _u32x4 a) {
#if defined(__arm__)
  return *r = vcvtq_f32_u32(a); /* NEON */
#elif defined(__x86__)
  _u32x4 one = {1, 1, 1, 1};
  _u32x4 half = a << one;
  _u32x4 low = a & one;
  half = half | low;
  _i32x4 as_signed = (_i32x4)half;
  _f32x4 two = {2.0f, 2.0f, 2.0f, 2.0f};
  _f32x4 to_float;
  return __pnacl_builtin_convertvector(&to_float, as_signed) * two;
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i32x4 __pnacl_builtin_convertvector(_i32x4* r, const _f32x4 a) {
#if defined(__arm__)
  return *r = vcvtq_s32_f32(a); /* NEON */
#elif defined(__x86__)
  return *r = _mm_cvtps_epi32(a); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u32x4 __pnacl_builtin_convertvector(_u32x4* r, const _f32x4 a) {
#if defined(__arm__)
  return *r = vcvtq_u32_f32(a); /* NEON */
#elif defined(__x86__)
  /* TODO: support for msb */
  return *r = _mm_cvtps_epi32(a); /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
int32_t __pnacl_builtin_signbits(const _f32x4 a) {
#if defined(__arm__)
  return 0; /* TODO: NEON */
#elif defined(__x86__)
  return _mm_movemask_ps(a);      /* SSE2 */
#endif
}

ALWAYS_INLINE
_i16x8 __pnacl_builtin_satpack(_i32x4 a, _i32x4 b) {
#if defined(__arm__)
  return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b)); /* NEON */
#elif defined(__x86__)
  return _mm_packs_epi32(a, b);   /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_i8x16 __pnacl_builtin_satpack(_i16x8 a, _i16x8 b) {
#if defined(__arm__)
  _i16x8 r = {0, 0, 0, 0, 0, 0, 0, 0};
  return r; /* TODO: NEON */
#elif defined(__x86__)
  return _mm_packs_epi16(a, b);   /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u16x8 __pnacl_builtin_usatpack(_u32x4 a, _u32x4 b) {
#if defined(__arm__)
  _i16x8 r = {0, 0, 0, 0, 0, 0, 0, 0};
  return r; /* TODO: NEON */
#elif defined(__x86__)
  return _mm_packus_epi32(a, b);  /* SSE4, TODO: SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

ALWAYS_INLINE
_u8x16 __pnacl_builtin_usatpack(_u16x8 a, _u16x8 b) {
#if defined(__arm__)
  _i16x8 r = {0, 0, 0, 0, 0, 0, 0, 0};
  return r; /* TODO: NEON */
#elif defined(__x86__)
  return _mm_packus_epi16(a, b);  /* SSE2 */
#else
#error "Reached unsupported configuration"
#endif
}

#endif
