/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test rounding behavior, checking and setting the rounding mode.
 */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

volatile double two = 2.0;
volatile double one = 1.0;
volatile double zero = 0.0;
volatile double neg_one = -1.0;
volatile double neg_two = -2.0;

#define ASSERT_TRUE(pred)                                     \
  if (!pred) {                                                \
    fprintf(stderr, "file %s, line %d, !(" #pred ")\n",       \
            __FILE__, __LINE__);                              \
  }

#define ASSERT_EQ(f1, f2)                                     \
  do {                                                        \
    double x = f1;                                            \
    double y = f2;                                            \
    if (x != y) {                                             \
      fprintf(stderr, "file %s, line %d, %f != %f\n",         \
              __FILE__, __LINE__, x, y);                      \
      abort();                                                \
    }                                                         \
  } while(0)

void __attribute__((noinline)) test_flt_rounds(int);
void test_flt_rounds(int expected_round) {
  ASSERT_EQ(FLT_ROUNDS, expected_round);
}

extern void set_round_toward_nearest(void);
extern void set_round_toward_plus_infinity(void);
extern void set_round_toward_minus_infinity(void);
extern void set_round_toward_zero(void);

/* Test that nearbyint() really does round to nearest. */
void __attribute__((noinline)) test_round_to_nearest(void);
void test_round_to_nearest(void) {
  ASSERT_EQ(nearbyint(one + 0.5), two);
  ASSERT_EQ(nearbyint(zero + 0.5), zero);
  ASSERT_EQ(nearbyint(zero), zero);
  ASSERT_EQ(nearbyint(-zero), -zero);
  ASSERT_EQ(nearbyint(neg_one + 0.5), -zero);
  ASSERT_TRUE(isnan(nearbyint(NAN)));
  ASSERT_EQ(nearbyint(INFINITY), INFINITY);
}

/* Test that nearbyint() really does round to plus infinity. */
void __attribute__((noinline)) test_round_to_plus_infinity(void);
void test_round_to_plus_infinity(void) {
  ASSERT_EQ(nearbyint(one + 0.5), two);
  ASSERT_EQ(nearbyint(zero + 0.5), one);
  ASSERT_EQ(nearbyint(zero), zero);
  ASSERT_EQ(nearbyint(-zero), -zero);
  ASSERT_EQ(nearbyint(neg_one + 0.5), -zero);
  ASSERT_TRUE(isnan(nearbyint(NAN)));
  ASSERT_EQ(nearbyint(INFINITY), INFINITY);
}

/* Test that nearbyint() really does round to minus infinity. */
void __attribute__((noinline)) test_round_to_minus_infinity(void);
void test_round_to_minus_infinity(void) {
  ASSERT_EQ(nearbyint(one + 0.5), one);
  ASSERT_EQ(nearbyint(zero + 0.5), zero);
  ASSERT_EQ(nearbyint(zero), zero);
  ASSERT_EQ(nearbyint(-zero), -zero);
  ASSERT_EQ(nearbyint(neg_one + 0.5), neg_one);
  ASSERT_TRUE(isnan(nearbyint(NAN)));
  ASSERT_EQ(nearbyint(INFINITY), INFINITY);
}

/* Test that nearbyint() really does round to zero. */
void __attribute__((noinline)) test_round_to_zero(void);
void test_round_to_zero(void) {
  ASSERT_EQ(nearbyint(one + 0.5), one);
  ASSERT_EQ(nearbyint(zero + 0.5), zero);
  ASSERT_EQ(nearbyint(zero), zero);
  ASSERT_EQ(nearbyint(-zero), -zero);
  ASSERT_EQ(nearbyint(neg_one + 0.5), -zero);
  ASSERT_TRUE(isnan(nearbyint(NAN)));
  ASSERT_EQ(nearbyint(INFINITY), INFINITY);
}

int main(int ac, char* av[]) {
#if defined(__mips__)
  /* For MIPS, the llvm.flt.rounds intrinsic is stuck at (1), so we cannot
   * test the other cases.  Furthermore, the llvm MIPS asm parser doesn't
   * parse the cfc1 $reg, $31 and ctc1 instructions for setting
   * the rounding mode.
   *
   * At least test that initial value is always round towards nearest.
   */
  test_flt_rounds(1);
  test_round_to_nearest();
#else
  /* Test that initial value is always round towards nearest. */
  test_flt_rounds(1);
  test_round_to_nearest();

  set_round_toward_plus_infinity();
  /*
   * GCC + Newlib and Glibc always return 1 for FLT_ROUNDS,
   * so we can't really test FLT_ROUNDS.  However, we can observe the
   * difference through nearbyint() behavior:
   * http://gcc.gnu.org/bugzilla/show_bug.cgi?id=30569
   */
#if defined(__clang__)
  test_flt_rounds(2);
#endif
  test_round_to_plus_infinity();

  set_round_toward_minus_infinity();
#if defined(__clang__)
  test_flt_rounds(3);
#endif
  test_round_to_minus_infinity();

  set_round_toward_zero();
#if defined(__clang__)
  test_flt_rounds(0);
#endif
  test_round_to_zero();

  set_round_toward_nearest();

#endif  /* __mips__ */
  test_flt_rounds(1);
  test_round_to_nearest();
  return 0;
}
