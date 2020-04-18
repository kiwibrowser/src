/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

/*
 * We want to be able to test LLVM's frem instruction without linking in fmod
 * at bitcode linking time to ensure that the native library has a
 * functioning copy of fmod. However, we also want to test that the bitcode
 * library's fmod also works. So we cannot test both fmod and frem in the
 * same build of the pexe, and instead build and test twice.
 */
#if defined(TEST_LLVM_IR)
extern double frem(double, double);
extern float fremf(float, float);
#define fmod frem
#define fmodf fremf
#endif

#define CHECK_ERRNO(expected)                           \
  do {                                                  \
    if (expected != errno) {                            \
      fprintf(stderr, "ERROR(%d): errno %d != %d\n",    \
              __LINE__, expected, errno);               \
      err_count++;                                      \
    }                                                   \
    /* Reset errno to something predictable. */         \
    errno = 0;                                          \
  } while(0)

#define CHECK_NAN(err, numer, denom)                            \
  do {                                                          \
    double res = fmod(numer, denom);                            \
    CHECK_ERRNO(err);                                           \
    if (!isnan(res)) {                                          \
      fprintf(stderr, "ERROR(%d): !isnan(%f mod %f) == %f\n",   \
              __LINE__, numer, denom, res);                     \
      err_count++;                                              \
    }                                                           \
  } while(0)

#define CHECK_NANF(err, numer, denom)                           \
  do {                                                          \
    float res = fmodf(numer, denom);                            \
    CHECK_ERRNO(err);                                           \
    if (!isnan(res)) {                                          \
      fprintf(stderr, "ERROR(%d): !isnan(%f modf %f) == %f\n",  \
              __LINE__, numer, denom, res);                     \
      err_count++;                                              \
    }                                                           \
  } while(0)

const double kTolerance = DBL_EPSILON;
const double kToleranceF = FLT_EPSILON * 2;

#define CHECK_EQ(expect, numer, denom)                                  \
  do {                                                                  \
    double res = fmod(numer, denom);                                    \
    CHECK_ERRNO(0);                                                     \
    /* The tolerance check may not work for SUBNORMAL, NaN, etc., so check */ \
    if (!(fpclassify(res) == FP_NORMAL || fpclassify(res) == FP_ZERO)) { \
      fprintf(stderr, "ERROR(%d): result is not normal/zero %f\n",      \
              __LINE__, res);                                           \
      err_count++;                                                      \
    }                                                                   \
    if (fabs(expect - res) > kTolerance) {                              \
      fprintf(stderr, "ERROR: %f mod %f == %f, != %f\n",                \
              numer, denom, res, expect);                               \
      err_count++;                                                      \
    }                                                                   \
  } while(0)


#define CHECK_EQF(expect, numer, denom)                                 \
  do {                                                                  \
    float res = fmodf(numer, denom);                                    \
    CHECK_ERRNO(0);                                                     \
    /* The tolerance check may not work for SUBNORMAL, NaN, etc., so check */ \
    if (!(fpclassify(res) == FP_NORMAL || fpclassify(res) == FP_ZERO)) { \
      fprintf(stderr, "ERROR(%d): result is not normal/zero %f\n",      \
              __LINE__, res);                                           \
      err_count++;                                                      \
    }                                                                   \
    if (fabsf(expect - res) > kToleranceF) {                            \
      fprintf(stderr, "ERROR: %f modf %f == %f, != %f\n",               \
              numer, denom, res, expect);                               \
      err_count++;                                                      \
    }                                                                   \
  } while(0)


int main(void) {
  /* Set up some volatile constants to block the optimizer. */
  volatile double zero = 0.0;
  volatile double nan = NAN;
  volatile double two = 2.0;
  volatile double onesix = 1.6;
  volatile double infinity = INFINITY;
  int err_count = 0;
#if defined(TEST_LLVM_IR)
  /* With the LLVM IR frem instruction, errno is never set
   * (see the PNaCl bitcode ABI documentation).
   */
  int expected_errno_infinity = 0;
  int expected_errno_zerodiv = 0;
#elif defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 10
  /*
   * The older (pre 2.10) glibc and newlib don't set errno when x is infinity.
   * See "BUGS" under the fmod manpage. It only sets errno for divide by zero.
   */
  int expected_errno_infinity = EDOM;
  int expected_errno_zerodiv = EDOM;
#else
  int expected_errno_infinity = 0;
  int expected_errno_zerodiv = EDOM;
#endif

  /* Initialize errno to something predictable. */
  errno = 0;

  /* If x or y is a NaN, a NaN is returned. */
  CHECK_NAN(0, nan, two);
  CHECK_NANF(0, (float)nan, (float)two);

  CHECK_NAN(0, two, nan);
  CHECK_NANF(0, (float)two, (float)nan);

  CHECK_NAN(0, -onesix, nan);
  CHECK_NANF(0, (float)-onesix, (float)nan);

  CHECK_NAN(0, nan, nan);
  CHECK_NANF(0, (float)nan, (float)nan);

  CHECK_NAN(0, nan, infinity);
  CHECK_NANF(0, (float)nan, (float)infinity);

  /* If x is infinity, a NaN is returned and errno is
   * expected_errno_infinity (see note about BUGS).
   */
  CHECK_NAN(expected_errno_infinity, infinity, two);
  CHECK_NANF(expected_errno_infinity, (float)infinity, (float)two);

  CHECK_NAN(expected_errno_infinity, -infinity, two);
  CHECK_NANF(expected_errno_infinity, (float)-infinity, (float)two);

  /* If y is zero, a NaN is returned and errno is expected_errno_zerodiv. */
  CHECK_NAN(expected_errno_zerodiv, two, zero);
  CHECK_NANF(expected_errno_zerodiv, (float)two, (float)zero);

  CHECK_NAN(expected_errno_zerodiv, two, -zero);
  CHECK_NANF(expected_errno_zerodiv, (float)two, (float)-zero);

  CHECK_NAN(expected_errno_zerodiv, infinity, zero);
  CHECK_NANF(expected_errno_zerodiv, (float)infinity, (float)zero);

  CHECK_NAN(expected_errno_zerodiv, infinity, -zero);
  CHECK_NANF(expected_errno_zerodiv, (float)infinity, (float)-zero);

  /* If x is +0 (-0), and y is not zero, a +0 (-0) is returned. */
  CHECK_EQ(zero, zero, two);
  CHECK_EQF((float)zero, (float)zero, (float)two);
  CHECK_EQ(zero, zero, -two);
  CHECK_EQF((float)zero, (float)zero, (float)-two);

  CHECK_EQ(-zero, -zero, two);
  CHECK_EQF((float)-zero, (float)-zero, (float)two);

  CHECK_EQ(-zero, -zero, -two);
  CHECK_EQF((float)-zero, (float)-zero, (float)-two);

  /*
   * On success... the returned value has the same sign as x and a magnitude
   * less than the magnitude of y.
   */
  CHECK_EQ(1.2, 5.2, two);
  CHECK_EQF(1.2f, 5.2f, (float)two);

  CHECK_EQ(-0.6, -0.6, two);
  CHECK_EQF(-0.6f, -0.6f, (float)two);

  CHECK_EQ(-0.6, -0.6, -two);
  CHECK_EQF(-0.6f, -0.6f, (float)-two);

  CHECK_EQ(zero, 6.4, onesix);
  CHECK_EQF((float)zero, 6.4, (float)onesix);

  CHECK_EQ(1.0, 5.0, two);
  CHECK_EQF(1.0f, 5.0, (float)two);
  CHECK_EQ(-1.0, -5.0, two);
  CHECK_EQF(-1.0f, -5.0, (float)two);

  CHECK_EQ(zero, 100.0, two);
  CHECK_EQF((float)zero, 100.0, (float)two);
  CHECK_EQ(-zero, -100.0, two);
  CHECK_EQF((float)-zero, -100.0, (float)two);

  /* If the numerator is finite and the denominator is an infinity, the
   * result is the numerator.
   */
  CHECK_EQ(5.2, 5.2, infinity);
  CHECK_EQF(5.2f, 5.2f, (float)infinity);

  CHECK_EQ(5.2, 5.2, -infinity);
  CHECK_EQF(5.2f, 5.2f, (float)-infinity);

  CHECK_EQ(-5.2, -5.2, infinity);
  CHECK_EQF(-5.2f, -5.2f, (float)infinity);

  fprintf(stderr, "Total of %d errors\n", err_count);
  return err_count;
}
