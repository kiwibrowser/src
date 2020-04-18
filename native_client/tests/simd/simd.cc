/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/nacl/simd.h"

#if !defined(NO_INLINE)
#define NO_INLINE inline __attribute__((noinline))
#endif

char* i8x16_to_a(char* buf, size_t sz, _i8x16 v) {
  snprintf(buf, sz, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", v[0],
           v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11],
           v[12], v[13], v[14], v[15]);
  return buf;
}

char* u8x16_to_a(char* buf, size_t sz, _u8x16 v) {
  snprintf(buf, sz,
           "0x%0X 0x%0X 0x%0X 0x%0X 0x%0X 0x%0X 0x%0X 0x%0X 0x%0X 0x%0X 0x%0X "
           "0x%0X 0x%0X 0x%0X 0x%0X 0x%0X ",
           v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10],
           v[11], v[12], v[13], v[14], v[15]);
  return buf;
}

bool is_equal(const _i8x16 a, const _i8x16 b) {
  return memcmp(&a, &b, sizeof(a)) ? false : true;
}

bool is_equal(const _u8x16 a, const _u8x16 b) {
  return memcmp(&a, &b, sizeof(a)) ? false : true;
}

bool is_equal(const _i32x4 a, const _i32x4 b) {
  return memcmp(&a, &b, sizeof(a)) ? false : true;
}

bool is_equal(const _f32x4 a, const _f32x4 b, const float ep) {
  _f32x4 d = __pnacl_builtin_abs(a - b);
  _f32x4 e = {ep, ep, ep, ep};
  _i32x4 c = d <= e;
  _i32x4 t = {-1, -1, -1, -1};
  return is_equal(t, c);
}

const char* pass_or_fail(bool b) { return b ? "PASSED" : "FAILED"; }

NO_INLINE
int8_t subtest_array_i8x16(_i8x16 a, int i) { return a[i]; }

bool test_array_i8x16() {
  const char* t = "test_array_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, 127};
  const _i8x16 b = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, 127};
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  bool r0 = true;
  for (int i = 0; i < 16; i++)
    r0 = r0 && (subtest_array_i8x16(a, i) == subtest_array_i8x16(b, i));
  printf("%s: a[i] == b[i]: %s\n", t, r0 ? "TRUE" : "FALSE");
  _i8x16 c;
  for (int i = 0; i < 16; i++) c[i] = a[i];
  printf("%s: c[i] = a[i]\n%s: c: %s\n", t, t, i8x16_to_a(buf, sizeof(buf), c));
  bool r1 = is_equal(a, c);
  printf("%s: a[i] == c[i]: %s\n", t, r1 ? "TRUE" : "FALSE");
  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_unary_minus_i8x16() {
  const char* t = "test_unary_minus_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, 127};
  const _i8x16 b = {0, 1, 2, 3, 4, 5, 6, 7, 0, -1, -2, -3, -4, -5, -6, -127};
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  _i8x16 c = -a;
  _i8x16 d = -b;
  printf("%s:       -a: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  bool r0 = is_equal(c, b);
  printf("%s:       -b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  bool r1 = is_equal(d, a);
  // test non-saturating overflow on -(-128)
  const _i8x16 e = {127, -128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  const _i8x16 f = {-127, -128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  _i8x16 g = -e;
  printf("%s: e: %s\n", t, i8x16_to_a(buf, sizeof(buf), e));
  printf("%s:       -e: %s\n", t, i8x16_to_a(buf, sizeof(buf), g));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), f));
  bool r2 = is_equal(f, g);
  bool r = r0 && r1 && r2;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_add_i8x16() {
  const char* t = "test_add_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, 127};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {0, -2, 0, -6, 0, -10, 0, -14, 0, 2, 0, 6, 0, 10, 0, -128};
  _i8x16 d = a + b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a + b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_sub_i8x16() {
  const char* t = "test_sub_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {0, 0, -4, 0, -8, 0, -12, 0, 0, 0, 4, 0, 8, 0, 12, 127};
  _i8x16 d = a - b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a - b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_mul_i8x16() {
  const char* t = "test_mul_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6,  -7,
                    0, 1,  2,  64, 64, 65, 127, 99};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 2, -2, 2, 1, 99};
  const _i8x16 c = {0, 1, -4, 9,    -16,  25,   -36, 49,
                    0, 1, -4, -128, -128, -126, 127, 73};
  _i8x16 d = a * b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a * b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

#if defined(VREFOPTIONAL)

NO_INLINE
_i8x16 subtest_div_i8x16(_i8x16 a, _i8x16 b) { return a / b; }

bool test_div_i8x16() {
  const char* t = "test_div_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -29, -29, 29, -12, -14,
                    0, 1,  2,  64,  64,  65, 127, -128};
  const _i8x16 b = {1, -1, 2, -10, 10, -10, 6, -7, 1, 1, -2, 2, -2, 2, 1, -1};
  const _i8x16 c = {0, 1, -1, 2, -2, -2, -2, 2, 0, 0, -1, 32, -32, 32, 127, 1};
  _i8x16 d = subtest_div_i8x16(a, b);
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a / b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

NO_INLINE
_i8x16 subtest_mod_i8x16(_i8x16 a, _i8x16 b) { return a / b; }

bool test_mod_i8x16() {
  const char* t = "test_mod_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -29, -29, 29, -12, -14,
                    0, 1,  2,  64,  64,  65, 127, 99};
  const _i8x16 b = {0, -1, 2, -10, 10, -10, 6, -7, 0, 0, -2, 2, -2, 2, 1, 99};
  const _i8x16 c = {0, 0, 0, -9, -9, 9, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
  _i8x16 d = subtest_mod_i8x16(a, b);
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a %% b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

#endif

bool test_bitwise_and_i8x16() {
  const char* t = "test_bitwise_and_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 0, -1, -1, -1, -1, 127, 2, 4, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 b = {0, -1, 0, -1, 2, 127, -1, 4, 2, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 c = {0, 0, 0, -1, 2, 127, 127, 0, 0, 6, 0, 1, 2, 4, 8, 127};
  _i8x16 d = a & b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a & b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_or_i8x16() {
  const char* t = "test_bitwise_or_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 0, -1, -1, -1, -1, 127, 2, 4, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 b = {0, -1, 0, -1, 2, 127, -1, 4, 2, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 c = {0, -1, -1, -1, -1, -1, -1, 6, 6, 6, 0, 1, 2, 4, 8, 127};
  _i8x16 d = a | b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a & b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_xor_i8x16() {
  const char* t = "test_bitwise_xor_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 0, -1, -1, -1, -1, 127, 2, 4, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 b = {0, -1, 0, -1, 2, 127, -1, 4, 2, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 c = {0, -1, -1, 0, -3, -128, -128, 6, 6, 0, 0, 0, 0, 0, 0, 0};
  _i8x16 d = a ^ b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a & b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_not_i8x16() {
  const char* t = "test_bitwise_not_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 0, -1, -1, -1, -1, 127, 2, 4, 6, 0, 1, 2, 4, 8, 127};
  const _i8x16 b = {-1, -1, 0,  0,  0,  0,  -128, -3,
                    -5, -7, -1, -2, -3, -5, -9,   -128};
  _i8x16 c = ~a;
  bool r = is_equal(b, c);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s:       ~a: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_shiftleft_i8x16() {
  const char* t = "test_shiftleft_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 0, -1, -1, -1, -1, 1, 2, 4, 31, 62, 127, 2, -1, -1, 127};
  const _i8x16 b = {0, 1, 1, 2, 3, 4, 1, 2, 4, 2, 3, 1, 2, 7, 8, 127};
  const _i8x16 c = {0,  0,   -2,  -4, -8, -16,  2, 8,
                    64, 124, -16, -2, 8,  -128, 0, 0};
  _i8x16 d = a << b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a << b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

NO_INLINE
_i8x16 subtest_shiftright_i8x16(_i8x16 a, _i8x16 b) { return a >> b; }

bool test_shiftright_i8x16() {
  const char* t = "test_shiftright_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 0,  -1, -128, -64, 64, 1,  2,
                    4, 31, 62, 31,   2,   -1, -1, 127};
  const _i8x16 b = {0, 1, 8, 8, 3, 3, 1, 2, 4, 2, 3, 1, 2, 7, 8, 127};
  const _i8x16 c = {0, 0, -1, -1, -8, 8, 0, 0, 0, 7, 7, 15, 0, -1, -1, 0};
  _i8x16 d = subtest_shiftright_i8x16(a, b);  // a >> b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a >> b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_less_i8x16() {
  const char* t = "test_less_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1};
  _i8x16 d = a < b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a < b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_lessequal_i8x16() {
  const char* t = "test_lessequal_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {-1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, 0,  -1, 0,  -1, 0,  -1};
  _i8x16 d = a <= b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a <= b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_greater_i8x16() {
  const char* t = "test_greater_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0};
  _i8x16 d = a > b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a > b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_greaterequal_i8x16() {
  const char* t = "test_greaterequal_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {-1, -1, 0, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0};
  _i8x16 d = a >= b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a >= b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_equal_i8x16() {
  const char* t = "test_equal_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {-1, -1, 0, -1, 0, -1, 0, -1, -1, -1, 0, -1, 0, -1, 0, 0};
  _i8x16 d = a == b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a == b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_notequal_i8x16() {
  const char* t = "test_notequal_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -6, 1};
  const _i8x16 c = {0, 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, -1};
  _i8x16 d = a != b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a != b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_assign_i8x16() {
  const char* t = "test_assign_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  _i8x16 b = a;
  bool r = is_equal(a, b);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s:    b = a: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_sizeof_i8x16() {
  const char* t = "test_sizeof_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 6, -128};
  size_t sz = sizeof(a);
  bool r = sz == 16;
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: sizeof(a): %d\n", t, (int)sz);
  printf("%s:  expected: 16\n", t);
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_shuffle_i8x16() {
  const char* t = "test_shufflevector_i8x16";
  char buf[1024];
  const _i8x16 a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  const _i8x16 b = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  _i8x16 c = __pnacl_builtin_shufflevector(a, a, 15, 14, 13, 12, 11, 10, 9, 8,
                                           7, 6, 5, 4, 3, 2, 1, 0);
  bool r0 = is_equal(b, c);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: shuffle(a, a, 15, 14, ... 1, 0): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), c));
  printf("%s:                        expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), b));
  const _i8x16 d = {20, 21, 22, 23, 24, 25, 26, 27,
                    28, 29, 30, 31, 32, 33, 34, 35};
  const _i8x16 e = {7, 6, 5, 4, 3, 2, 1, 0, 20, 21, 22, 23, 24, 25, 26, 27};
  _i8x16 f = __pnacl_builtin_shufflevector(a, d, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17,
                                           18, 19, 20, 21, 22, 23);
  bool r1 = is_equal(f, e);
  printf("%s: d: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: shuffle(a, d, 7, 6, ... 0, 16, 17, ...23): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), f));
  printf("%s:                        expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), e));
  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_load_i8x16() {
  const char* t = "test_load_i8x16";
  char buf[1024];
  char mem[sizeof(_i8x16) * 2];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 127, -128};
  _i8x16 b;
  memcpy(&mem[0], &a, sizeof(a));
  b = __pnacl_builtin_load(&b, &mem[0]);
  bool r0 = is_equal(a, b);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_load(mem[0]): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), a));

  memset(mem, 0, sizeof(mem));
  memcpy(&mem[1], &a, sizeof(a));
  b = __pnacl_builtin_load(&b, &mem[1]);
  bool r1 = is_equal(a, b);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_load(mem[1]): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), a));
  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_store_i8x16() {
  const char* t = "test_store_i8x16";
  char buf[1024];
  char mem[sizeof(_i8x16) * 2];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 127, -128};
  _i8x16 b;
  memset(mem, 0, sizeof(mem));
  __pnacl_builtin_store(&mem[0], a);
  b = __pnacl_builtin_load(&b, &mem[0]);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_store(mem[0]): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), a));
  bool r0 = 0 == memcmp(&mem[0], &a, sizeof(a)) && is_equal(a, b);

  memset(mem, 0, sizeof(mem));
  __pnacl_builtin_store(&mem[1], a);
  b = __pnacl_builtin_load(&b, &mem[1]);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_store(mem[1]): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), a));
  bool r1 = 0 == memcmp(&mem[1], &a, sizeof(a)) && is_equal(a, b);

  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_min_i8x16() {
  const char* t = "test_min_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 127, -128};
  const _i8x16 b = {4, -4, -4, -2, 20, -9, 20, -128, 127, 2, 2, 2, 0, 0, 0, 0};
  const _i8x16 c = {0, -4, -4, -3, -4, -9, -6, -128, 0, 1, 2, 2, 0, 0, 0, -128};
  _i8x16 d = __pnacl_builtin_min(a, b);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_min(a,b): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                 expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), c));
  bool r = is_equal(c, d);
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_max_i8x16() {
  const char* t = "test_max_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 127, -128};
  const _i8x16 b = {4, -4, -4, -2, 20, -9, 20, -128, 127, 2, 2, 2, 0, 0, 0, 0};
  const _i8x16 c = {4, -1, -2, -2, 20, -5, 20, -7, 127, 2, 2, 3, 4, 5, 127, 0};
  _i8x16 d = __pnacl_builtin_max(a, b);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_max(a,b): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                 expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), c));
  bool r = is_equal(c, d);
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_satadd_i8x16() {
  const char* t = "test_satadd_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, -128, 127};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -1, 1};
  const _i8x16 c = {0, -2, 0, -6, 0, -10, 0, -14, 0, 2, 0, 6, 0, 10, -128, 127};
  _i8x16 d = __pnacl_builtin_satadd(a, b);
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_satadd(a, b): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                     expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_satsub_i8x16() {
  const char* t = "test_satsub_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6, -7, 0, 1, 2, 3, 4, 5, 127, -128};
  const _i8x16 b = {0, -1, 2, -3, 4, -5, 6, -7, 0, 1, -2, 3, -4, 5, -1, 1};
  const _i8x16 c = {0, 0, -4, 0, -8, 0, -12, 0, 0, 0, 4, 0, 8, 0, 127, -128};
  _i8x16 d = __pnacl_builtin_satsub(a, b);
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_satsub(a, b): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                     expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_abs_i8x16() {
  const char* t = "test_abs_i8x16";
  char buf[1024];
  const _i8x16 a = {0, -1, -2, -3, -4, -5, -6,   -7,
                    0, 1,  2,  3,  4,  5,  -127, -128};
  const _i8x16 b = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 127, -128};
  _i8x16 c = __pnacl_builtin_abs(a);
  bool r = is_equal(b, c);
  printf("%s: a: %s\n", t, i8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_abs(a): %s\n", t,
         i8x16_to_a(buf, sizeof(buf), c));
  printf("%s:               expected: %s\n", t,
         i8x16_to_a(buf, sizeof(buf), b));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

// ---------

NO_INLINE
uint8_t subtest_array_u8x16(_u8x16 a, int i) { return a[i]; }

bool test_array_u8x16() {
  const char* t = "test_array_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  const _u8x16 b = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  bool r0 = true;
  for (int i = 0; i < 16; i++)
    r0 = r0 && (subtest_array_u8x16(a, i) == subtest_array_u8x16(b, i));
  printf("%s: a[i] == b[i]: %s\n", t, r0 ? "TRUE" : "FALSE");
  _u8x16 c;
  for (int i = 0; i < 16; i++) c[i] = a[i];
  printf("%s: c[i] = a[i]\n%s: c: %s\n", t, t, u8x16_to_a(buf, sizeof(buf), c));
  bool r1 = is_equal(a, c);
  printf("%s: a[i] == c[i]: %s\n", t, r1 ? "TRUE" : "FALSE");
  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_add_u8x16() {
  const char* t = "test_add_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  const _u8x16 b = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0xFF};
  const _u8x16 c = {0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,
                    0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0xE};
  _u8x16 d = a + b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a + b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_sub_u8x16() {
  const char* t = "test_sub_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0xFF, 0xFF, 0x20, 0xFF, 0x00, 0xFE, 0x70,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  const _u8x16 b = {0x00, 0xFF, 0x7F, 0x10, 0x01, 0x01, 0xFF, 0x10,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 c = {0x00, 0x00, 0x80, 0x10, 0xFE, 0xFF, 0xFF, 0x60,
                    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1};
  _u8x16 d = a - b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a - b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_mul_u8x16() {
  const char* t = "test_mul_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 b = {0x00, 0x10, 0x11, 0x12, 0x13, 0x02, 0x03, 0x04,
                    0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x20, 0x30};
  const _u8x16 c = {0x00, 0x10, 0x22, 0x36, 0x4C, 0x0A, 0x12, 0x1C,
                    0x00, 0xFF, 0xFE, 0xFD, 0xFC, 0x7B, 0xC0, 0x50};
  _u8x16 d = a * b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a * b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_and_u8x16() {
  const char* t = "test_bitwise_and_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x02,
                    0x04, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 b = {0x00, 0xFF, 0x00, 0xFF, 0x02, 0x7F, 0xFF, 0x04,
                    0x02, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 c = {0x00, 0x00, 0x00, 0xFF, 0x02, 0x7F, 0x7F, 0x00,
                    0x00, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  _u8x16 d = a & b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a & b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_or_u8x16() {
  const char* t = "test_bitwise_or_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x02,
                    0x04, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 b = {0x00, 0xFF, 0x00, 0xFF, 0x02, 0x7F, 0xFF, 0x04,
                    0x02, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 c = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x06,
                    0x06, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  _u8x16 d = a | b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a & b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_xor_u8x16() {
  const char* t = "test_bitwise_xor_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x02,
                    0x04, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 b = {0x00, 0xFF, 0x00, 0xFF, 0x02, 0x7F, 0xFF, 0x04,
                    0x02, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 c = {0x00, 0xFF, 0xFF, 0x00, 0xFD, 0x80, 0x80, 0x06,
                    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  _u8x16 d = a ^ b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a & b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_bitwise_not_u8x16() {
  const char* t = "test_bitwise_not_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x02,
                    0x04, 0x06, 0x00, 0x01, 0x02, 0x04, 0x08, 0x7F};
  const _u8x16 b = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFD,
                    0xFB, 0xF9, 0xFF, 0xFE, 0xFD, 0xFB, 0xF7, 0x80};
  _u8x16 c = ~a;
  bool r = is_equal(b, c);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s:       ~a: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_shiftleft_u8x16() {
  const char* t = "test_shiftleft_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x02,
                    0x04, 0x21, 0x40, 0x7F, 0x02, 0xFF, 0xFF, 0x7F};
  const _u8x16 b = {0, 1, 1, 2, 3, 4, 1, 2, 4, 2, 3, 1, 2, 7, 8, 127};
  const _u8x16 c = {0x00, 0x00, 0xFE, 0xFC, 0xF8, 0xF0, 0x02, 0x08,
                    0x40, 0x84, 0x00, 0xFE, 0x08, 0x80, 0x00, 0x00};
  _u8x16 d = a << b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a << b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_shiftright_u8x16() {
  const char* t = "test_shiftright_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0xFF, 0x80, 0x40, 0x20, 0x01, 0x02,
                    0x04, 0x21, 0x40, 0x7F, 0x02, 0xFF, 0xFF, 0x7F};
  const _u8x16 b = {0, 1, 8, 8, 3, 3, 1, 2, 4, 2, 3, 1, 2, 7, 8, 127};
  const _u8x16 c = {0x00, 0x00, 0x00, 0x00, 0x08, 0x04, 0x00, 0x00,
                    0x00, 0x08, 0x08, 0x3F, 0x00, 0x01, 0x00, 0x00};
  _u8x16 d = a >> b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a >> b: %s\n", t, u8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_less_u8x16() {
  const char* t = "test_less_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x04, 0xFE, 0x06,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 b = {0x00, 0xFF, 0xFF, 0xFE, 0x03, 0x7F, 0xFF, 0x40,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF};
  const _i8x16 c = {0, 0, -1, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, -1, -1};
  _i8x16 d = a < b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a < b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_lessequal_u8x16() {
  const char* t = "test_lessequal_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x04, 0xFE, 0x06,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 b = {0x00, 0xFF, 0xFF, 0xFE, 0x03, 0x7F, 0xFF, 0x40,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF};
  const _i8x16 c = {-1, -1, -1, 0,  0,  -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1};
  _i8x16 d = a <= b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a <= b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_greater_u8x16() {
  const char* t = "test_greater_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x04, 0xFE, 0x06,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 b = {0x00, 0xFF, 0xFF, 0xFE, 0x03, 0x7F, 0xFF, 0x40,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF};
  const _i8x16 c = {0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  _i8x16 d = a > b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:    a > b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_greaterequal_u8x16() {
  const char* t = "test_greaterequal_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x04, 0xFE, 0x06,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 b = {0x00, 0xFF, 0xFF, 0xFE, 0x03, 0x7F, 0xFF, 0x40,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF};
  const _i8x16 c = {-1, -1, 0, -1, -1, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0};
  _i8x16 d = a >= b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a >= b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_equal_u8x16() {
  const char* t = "test_equal_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x04, 0xFE, 0x06,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  const _u8x16 b = {0x00, 0xFF, 0xFF, 0xFE, 0x03, 0x7F, 0xFF, 0x40,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF};
  const _i8x16 c = {-1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, 0, 0, 0};
  _i8x16 d = a == b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a == b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_notequal_u8x16() {
  const char* t = "test_notequal_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x7F};
  const _u8x16 b = {0x00, 0xFF, 0x02, 0xFE, 0x04, 0xFC, 0xFF, 0xFF,
                    0x00, 0x01, 0x02, 0xFF, 0x05, 0x05, 0x06, 0xFF};
  const _i8x16 c = {0, -1, 0, -1, 0, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1};
  _i8x16 d = a != b;
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:   a != b: %s\n", t, i8x16_to_a(buf, sizeof(buf), d));
  printf("%s: expected: %s\n", t, i8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_assign_u8x16() {
  const char* t = "test_assign_u8x16";
  char buf[1024];
  const _u8x16 a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 255};
  _u8x16 b = a;
  bool r = is_equal(a, b);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s:    b = a: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s: expected: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_sizeof_u8x16() {
  const char* t = "test_sizeof_u8x16";
  char buf[1024];
  const _u8x16 a = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 255};
  size_t sz = sizeof(a);
  bool r = sz == 16;
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: sizeof(a): %d\n", t, (int)sz);
  printf("%s:  expected: 16\n", t);
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_shuffle_u8x16() {
  const char* t = "test_shufflevector_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  const _u8x16 b = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
                    0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
  _u8x16 c = __pnacl_builtin_shufflevector(a, a, 15, 14, 13, 12, 11, 10, 9, 8,
                                           7, 6, 5, 4, 3, 2, 1, 0);
  bool r0 = is_equal(b, c);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: shuffle(a, a, 15, 14, ... 1, 0): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), c));
  printf("%s:                        expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), b));
  const _u8x16 d = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
                    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F};
  const _u8x16 e = {0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
                    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};
  _u8x16 f = __pnacl_builtin_shufflevector(a, d, 7, 6, 5, 4, 3, 2, 1, 0, 16, 17,
                                           18, 19, 20, 21, 22, 23);
  bool r1 = is_equal(f, e);
  printf("%s: d: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: shuffle(a, d, 7, 6, ... 0, 16, 17, ...23): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), f));
  printf("%s:                                  expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), e));
  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_load_u8x16() {
  const char* t = "test_load_u8x16";
  char buf[1024];
  char mem[sizeof(_u8x16) * 2];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
  _u8x16 b;
  memcpy(&mem[0], &a, sizeof(a));
  b = __pnacl_builtin_load(&b, &mem[0]);
  bool r0 = is_equal(a, b);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_load(mem[0]): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), a));

  memset(mem, 0, sizeof(mem));
  memcpy(&mem[1], &a, sizeof(a));
  b = __pnacl_builtin_load(&b, &mem[1]);
  bool r1 = is_equal(a, b);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_load(mem[1]): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), a));
  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_store_u8x16() {
  const char* t = "test_store_u8x16";
  char buf[1024];
  char mem[sizeof(_u8x16) * 2];
  const _u8x16 a = {0x00, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
  _u8x16 b;
  memset(mem, 0, sizeof(mem));
  __pnacl_builtin_store(&mem[0], a);
  b = __pnacl_builtin_load(&b, &mem[0]);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_store(mem[0]): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), a));
  bool r0 = 0 == memcmp(&mem[0], &a, sizeof(a)) && is_equal(a, b);

  memset(mem, 0, sizeof(mem));
  __pnacl_builtin_store(&mem[1], a);
  b = __pnacl_builtin_load(&b, &mem[1]);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: __pnacl_builtin_store(mem[1]): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), b));
  printf("%s:                     expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), a));
  bool r1 = 0 == memcmp(&mem[1], &a, sizeof(a)) && is_equal(a, b);

  bool r = r0 && r1;
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_min_u8x16() {
  const char* t = "test_min_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0xFF};
  const _u8x16 b = {0x01, 0xFF, 0xFE, 0x03, 0x00, 0x06, 0x01, 0x08,
                    0x09, 0x09, 0x0B, 0x99, 0x13, 0x15, 0x16, 0x00};
  const _u8x16 c = {0x00, 0x00, 0x02, 0x03, 0x00, 0x05, 0x01, 0x07,
                    0x08, 0x09, 0x0B, 0x11, 0x12, 0x13, 0x14, 0x00};
  _u8x16 d = __pnacl_builtin_min(a, b);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_min(a,b): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                 expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), c));
  bool r = is_equal(c, d);
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_max_u8x16() {
  const char* t = "test_max_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0xFF};
  const _u8x16 b = {0x01, 0xFF, 0xFE, 0x03, 0x00, 0x06, 0x01, 0x08,
                    0x09, 0x09, 0x0B, 0x99, 0x13, 0x15, 0x16, 0x00};
  const _u8x16 c = {0x01, 0xFF, 0xFE, 0x03, 0x04, 0x06, 0x06, 0x08,
                    0x09, 0x09, 0x10, 0x99, 0x13, 0x15, 0x16, 0xFF};
  _u8x16 d = __pnacl_builtin_max(a, b);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_max(a,b): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                 expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), c));
  bool r = is_equal(c, d);
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_satadd_u8x16() {
  const char* t = "test_satadd_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x10, 0x20, 0x30, 0xFF, 0x7F, 0xFF};
  const _u8x16 b = {0x00, 0x01, 0x01, 0x04, 0x00, 0x05, 0x00, 0x07,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x00, 0xFF, 0x01};
  const _u8x16 c = {0x00, 0x02, 0x03, 0x07, 0x04, 0x0A, 0x06, 0x0E,
                    0x08, 0x0A, 0x12, 0x23, 0x34, 0xFF, 0xFF, 0xFF};
  _u8x16 d = __pnacl_builtin_satadd(a, b);
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_satadd(a, b): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                     expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

bool test_satsub_u8x16() {
  const char* t = "test_satsub_u8x16";
  char buf[1024];
  const _u8x16 a = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x10, 0x20, 0x30, 0xFF, 0x7F, 0xFF};
  const _u8x16 b = {0x00, 0x01, 0x01, 0x04, 0x00, 0x05, 0x00, 0x07,
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x00, 0xFF, 0x01};
  const _u8x16 c = {0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x06, 0x00,
                    0x08, 0x08, 0x0E, 0x1D, 0x2C, 0xFF, 0x00, 0xFE};
  _u8x16 d = __pnacl_builtin_satsub(a, b);
  bool r = is_equal(c, d);
  printf("%s: a: %s\n", t, u8x16_to_a(buf, sizeof(buf), a));
  printf("%s: b: %s\n", t, u8x16_to_a(buf, sizeof(buf), b));
  printf("%s: __pnacl_builtin_satsub(a, b): %s\n", t,
         u8x16_to_a(buf, sizeof(buf), d));
  printf("%s:                     expected: %s\n", t,
         u8x16_to_a(buf, sizeof(buf), c));
  printf("%s: %s\n\n", t, pass_or_fail(r));
  return r;
}

int main() {
  bool all = true;

  // tests for signed int8_t x 16 vector type
  all &= test_array_i8x16();
  all &= test_unary_minus_i8x16();
  all &= test_add_i8x16();
  all &= test_sub_i8x16();
  all &= test_mul_i8x16();
  all &= test_bitwise_and_i8x16();
  all &= test_bitwise_or_i8x16();
  all &= test_bitwise_xor_i8x16();
  all &= test_bitwise_not_i8x16();
  all &= test_shiftleft_i8x16();
  all &= test_shiftright_i8x16();
  all &= test_less_i8x16();
  all &= test_lessequal_i8x16();
  all &= test_greater_i8x16();
  all &= test_greaterequal_i8x16();
  all &= test_equal_i8x16();
  all &= test_notequal_i8x16();
  all &= test_sizeof_i8x16();
  all &= test_shuffle_i8x16();
  all &= test_load_i8x16();
  all &= test_store_i8x16();
  all &= test_min_i8x16();
  all &= test_max_i8x16();
  all &= test_satadd_i8x16();
  all &= test_satsub_i8x16();
  all &= test_abs_i8x16();
#if defined(VREFOPTIONAL)
  all &= test_div_i8x16();
  all &= test_mod_i8x16();
#endif

  // tests for unsigned uint8_t x 16 vector type
  all &= test_array_u8x16();
  all &= test_add_u8x16();
  all &= test_sub_u8x16();
  all &= test_mul_u8x16();
  all &= test_bitwise_and_u8x16();
  all &= test_bitwise_or_u8x16();
  all &= test_bitwise_xor_u8x16();
  all &= test_bitwise_not_u8x16();
  all &= test_shiftleft_u8x16();
  all &= test_shiftright_u8x16();
  all &= test_less_u8x16();
  all &= test_lessequal_u8x16();
  all &= test_greater_u8x16();
  all &= test_greaterequal_u8x16();
  all &= test_equal_u8x16();
  all &= test_notequal_u8x16();
  all &= test_sizeof_u8x16();
  all &= test_shuffle_u8x16();
  all &= test_load_u8x16();
  all &= test_store_u8x16();
  all &= test_min_u8x16();
  all &= test_max_u8x16();
  all &= test_satadd_u8x16();
  all &= test_satsub_u8x16();
  // TODO(nfullagar): test for __pnacl_builtin_avg

  // TODO(nfullagar): i16x8, u16x8, i32x4, u32x4, f32x4 types.

  printf("\nCombined result: %s\n\n",
         all ? "all tests PASSED!" : "one or more tests FAILED.");

  return 0;
}
