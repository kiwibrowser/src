/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test parts of math.h and floating point ops for compliance against ieee754.
 */

#include <math.h>
#include <stdio.h>
#include <errno.h>
/* #include <fenv.h>
 * TODO(jvoung) Newlib doesn't have this on any machine, except the Cell SPU.
 * Would like to test the exception flagging (see below).
 */

#define CHECK_EQ_DOUBLE(a, b)                                           \
  (a == b) ? 0 : (printf("ERROR %f != %f at %d\n", a, b, __LINE__), 1)

#define CHECK_NAN(_x_)                                                    \
  isnan(_x_) ? 0 : (printf("ERROR %s(%f) != NaN at %d\n", \
  #_x_, (double)_x_, __LINE__), 1)

#define CHECK_INF(a)                                                    \
  isinf(a) ? 0 : (printf("ERROR %f != Inf at %d\n", (double)a, __LINE__), 1)

#define ASSERT_TRUE(b)                                                  \
  (b) ? 0 : (printf("ASSERT true failed! %d at %d\n", b, __LINE__), 1)

#define ASSERT_FALSE(b)                                                 \
  (0 == (b)) ? 0 : (printf("ASSERT false failed! %d at %d\n", b, __LINE__), 1)

/************************************************************/

int test_constants(void) {
  int errs = 0;
  /* Attempt to prevent constant folding */
  volatile double x = 1.0;
  volatile double y = 0.0;
  errs += CHECK_NAN(NAN);
  printf("Print out of NaN: %f\n", NAN);
  errs += CHECK_INF(INFINITY);
  printf("Print out of Infinity: %f\n", INFINITY);
  errs += CHECK_INF(x/y);
  return errs;
}

int test_compares(void) {
  int errs = 0;
  /* Attempt to prevent constant folding */
  volatile double x;
  volatile double y;

  printf("Comparing float constants\n");
  x = NAN;
  errs += ASSERT_TRUE(x != x);
  errs += ASSERT_TRUE(isunordered(x, x));
  errs += CHECK_NAN(x + 3.0f);
  errs += CHECK_NAN(x + x);
  errs += CHECK_NAN(x - x);
  errs += CHECK_NAN(x / x);
  errs += CHECK_NAN(0.0 / x);
  errs += CHECK_NAN(0.0 * x);

  errs += ASSERT_FALSE(x == x);

  x = INFINITY;
  errs += ASSERT_TRUE(x == x);
  errs += ASSERT_FALSE(x == -x);
  errs += ASSERT_TRUE(-x == -x);
  errs += ASSERT_TRUE(x + 100.0 == x);
  errs += ASSERT_TRUE(x - 100.0 == x);
  errs += ASSERT_TRUE(-x - 100.0 == -x);
  errs += ASSERT_TRUE(-x + 100.0 == -x);
  errs += ASSERT_TRUE(-x < x);
  errs += ASSERT_FALSE(-x > x);

  y = 0.0;
  errs += CHECK_NAN(y * x);
  errs += CHECK_NAN(y / y);
  errs += CHECK_NAN(x / x);
  errs += CHECK_NAN(x - x);
  y = NAN;
  errs += CHECK_NAN(x * y);

  x = INFINITY;
  errs += CHECK_INF(x + x);
  x = 1.0; y = 0.0;
  errs += CHECK_INF(x / y);

  x = INFINITY;
  errs += ASSERT_FALSE(isfinite(x));
  x = NAN;
  errs += ASSERT_FALSE(isfinite(x));

  return errs;
}

/* Test non-NaN-resulting library calls. */
int test_defined(void) {
  int errs = 0;
  /*
   * Attempt to prevent constant folding and optimization of library
   * function bodies (when statically linked).
   */
  volatile double x;
  volatile double y;
  volatile double z;

  printf("Checking lib calls that take NaN, etc, but return non-NaN.\n");
  x = 0.0; y = 1.0;
  errs += CHECK_EQ_DOUBLE(pow(x, x), y);
  z = NAN;
  errs += CHECK_EQ_DOUBLE(pow(z, x), y);
  z = INFINITY;
  errs += CHECK_EQ_DOUBLE(pow(z, x), y);

  errs += CHECK_INF(sqrt(z));
  x = -0.0;
  errs += CHECK_EQ_DOUBLE(sqrt(x), x);
  x = INFINITY; y = 2.0;
  errs += CHECK_INF(pow(x, y));
  x = 0.0; y = -INFINITY;
  errs += ASSERT_TRUE(log(x) == y);

  return errs;
}

/* Test NaN-resulting library calls. */
int test_errs(void) {
  int errs = 0;
  /*
   * Attempt to prevent constant folding and optimization of library
   * function bodies (when statically linked).
   */
  volatile double x;
  volatile double y;

  printf("Checking well-defined library errors\n");
  x = -3.0; y = 4.4;
  errs += CHECK_NAN(pow(x, y));
  errs += CHECK_NAN(log(x));
  x = -0.001;
  errs += CHECK_NAN(sqrt(x));
  x = 1.0001;
  errs += CHECK_NAN(asin(x));
  x = INFINITY;
  errs += CHECK_NAN(sin(x));
  errs += CHECK_NAN(cos(x));
  x = 0.999;
  errs += CHECK_NAN(acosh(x));
  x = 3.3; y = 0.0;
  errs += CHECK_NAN(remainder(x, y));
  y = INFINITY;
  errs += CHECK_NAN(remainder(y, x));
  return errs;
}

/*
  TODO(jvoung) Check if exceptions are flagged in status word
  (once fenv.h is in newlib, or we move to using glibc)?

test FE_INVALID
test FE_DIVBYZERO
test FE_INEXACT
test FE_OVERFLOW
test FE_UNDERFLOW

E.g., Ordered comparisons of NaN should raise INVALID
  (NAN < NAN);
  (NAN >= NAN);
  (1.0 >= NAN);

  as, should all the calls in test_errs()

Some notes:
http://www.kernel.org/doc/man-pages/online/pages/man7/math_error.7.html
https://www.securecoding.cert.org/confluence/display/seccode/ \
        FLP03-C.+Detect+and+handle+floating+point+errors

*/

/* Check exceptions communicated by the "old" errno mechanism */
#define CHECK_TRIPPED_ERRNO(expr)                             \
  (errno = 0,                                                 \
   (void)expr,                                                \
   (0 != errno ? 0 : (printf("%s - errno %d not set at %d\n", \
                      #expr, errno, __LINE__), 1)))

int test_exception(void) {
  int errs = 0;
  /*
   * Attempt to prevent constant folding and optimization of library
   * function bodies (when statically linked).
   */
  volatile double x;
  volatile double y;

  printf("Checking that exceptional lib calls set errno\n");
  x = -3.0; y = 4.4;
  errs += CHECK_TRIPPED_ERRNO(pow(x, y));
  errs += CHECK_TRIPPED_ERRNO(log(x));
  x = -0.001;
  errs += CHECK_TRIPPED_ERRNO(sqrt(x));
  x = 1.0001;
  errs += CHECK_TRIPPED_ERRNO(asin(x));

/* Some versions of libc don't set errno =(
 * http://sourceware.org/bugzilla/show_bug.cgi?id=6780
 * http://sourceware.org/bugzilla/show_bug.cgi?id=6781
  x = INFINITY;
  errs += CHECK_TRIPPED_ERRNO(sin(x));
  errs += CHECK_TRIPPED_ERRNO(cos(x));
 */
  x = 0.999;
  errs += CHECK_TRIPPED_ERRNO(acosh(x));
  x = 3.3; y = 0.0;
  errs += CHECK_TRIPPED_ERRNO(remainder(x, y));

/* Some versions of libc don't set errno =(
 * http://sourceware.org/bugzilla/show_bug.cgi?id=6783
  x = INFINITY; y = 3.3;
  errs += CHECK_TRIPPED_ERRNO(remainder(x, y));
 */
  return errs;
}


int main(int ac, char* av[]) {
  int errs = 0;

  errs += test_constants();
  errs += test_compares();
  errs += test_defined();
  errs += test_errs();
#if !defined(NO_ERRNO_CHECK)
  errs += test_exception();
#endif

  printf("%d errs!\n", errs);
  return errs + 55;
}
