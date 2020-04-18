/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that the front/middle/backends can deal with the
 * math.h functions which have builtins and llvm intrinsics.
 *
 * There are two categories.
 *
 * (1) Standard uses without resorting to llvm assembly:
 * direct calls to C library function, or the gcc __builtin_sin(), etc.
 *
 * (2) Directly testing llvm.* intrinsics. These we test when we know
 * that they *will* be in bitcode or are useful (e.g., we know hardware
 * acceleration is available).  We currently use "llvm.sqrt.*" within
 * the libm sqrt() function: newlib-trunk/newlib/libm/machine/pnacl/w_sqrt.c
 * The libm sqrt is coded to have error checking and errno setting external
 * to the llvm.sqrt.* call.  Other uses of llvm.sqrt have no guarantees
 * about setting errno. This tests that we don't get infinite recursion
 * from such usage (e.g., if a backend expands llvm.sqrt.f64 within sqrt()
 * back into a call to sqrt()).
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "native_client/tests/toolchain/utils.h"

typedef float v4f32 __attribute__((vector_size(16)));

/* Volatile to prevent library-call constant folding optimizations. */
volatile float f32[] = {-NAN, NAN, -INFINITY, -HUGE_VALF,
                        -M_E, -M_PI_2, -M_PI, -16.0, -0.5, -0.0,
                        0.0, 5.0, 16.0, 10.0, M_PI, M_PI_2, M_E,
                        HUGE_VALF, INFINITY };
volatile double f64[] = {-NAN, NAN, -INFINITY, -HUGE_VAL,
                         -M_E, -M_PI_2, -M_PI, -16.0, -0.5, -0.0,
                         0.0, 5.0, 16.0, 10.0, M_PI, M_PI_2, M_E,
                         HUGE_VAL, INFINITY};

volatile float base32 = 2.0;
volatile float neg_base32 = -2.0;
volatile double base64 = 2.0;
volatile double neg_base64 = -2.0;

/*
 * The LLVM language reference considers this undefined for values < -0.0.
 * In practice hardware sqrt instructions returns NaN in that case.
 * We will need to guarantee this behavior.
 */
float llvm_intrinsic_sqrtf(float) __asm__("llvm.sqrt.f32");
double llvm_intrinsic_sqrt(double) __asm__("llvm.sqrt.f64");

/*
 * Floating point abs should always clear the sign bit. E.g., from -nan to nan
 * and -inf to inf.
 */
float llvm_intrinsic_fabsf(float) __asm__("llvm.fabs.f32");
double llvm_intrinsic_fabs(double) __asm__("llvm.fabs.f64");
v4f32 llvm_intrinsic_vec_fabs(v4f32) __asm__("llvm.fabs.v4f32");

/*
 * Normally, printf can end up printing NAN and INFINITY values
 * differently depending on the libc.
 *
 * C99 7.19.6.1 The fprintf function, paragraph 8, section of the f,F
 * modifiers says the following:
 *
 * A double argument representing an infinity is converted in one of
 * the styles [-]inf or [-]infinity — which style is implementation-defined.
 * A double argument representing a NaN is converted in one of the styles
 * [-]nan or [-]nan(n-char-sequence) — which style, and the meaning of
 * any n-char-sequence, is implementation-defined. The F conversion
 * specifier produces INF, INFINITY, or NAN instead of inf, infinity,
 * or nan, respectively.
 *
 * This custom routine works around a newlib bug:
 * https://code.google.com/p/nativeclient/issues/detail?id=4039
 * TODO(jvoung): remove the workaround when newlib is fixed.
 *
 * Also, there are some cases where the intrinsics and functions
 * currently return different values on different architectures
 * (see test cases below). The |nan_sign| parameter should be true
 * if it's okay to print the sign of the nan for the golden output
 * (consistent / no known platform difference), and false otherwise.
 */
template <typename T>
static int sprint_fp(char *buf, const char *format_with_prec,
                     T x, int nan_sign) {
  if (isnan(x)) {
    if (nan_sign && signbit(x) != 0)
      return sprintf(buf, "-nan");
    else
      return sprintf(buf, "nan");
  } else if (isinf(x)) {
    if (signbit(x) != 0)
      return sprintf(buf, "-inf");
    else
      return sprintf(buf, "inf");
  } else {
    return sprintf(buf, format_with_prec, x);
  }
}

/* Print a v4f32 vector to a string buffer. Should probably rewrite this
 * to C++ and use templates if we need to test more vector variants.
 */
static void sprint_v4f32(char *buf, const char *format_with_prec,
                         v4f32 vec, int nan_sign) {
  size_t num_elems = sizeof(vec) / sizeof(vec[0]);
  for (int i = 0; i < num_elems; ++i) {
    if (i != 0) {
      int r = sprintf(buf, ", ");
      buf += r;
    }
    int r = sprint_fp(buf, format_with_prec, vec[i], nan_sign);
    buf += r;
  }
}


/* NOTE: These macros depend on a "sprint_buf" local var. */

#define print_op1_libm(prec, op, x, ns)                      \
  do {                                                       \
    __typeof(x) res = op(x);                                 \
    sprint_fp(sprint_buf, "%." #prec "f", res, ns);          \
    printf("%s (math.h): %s\n", #op, sprint_buf);            \
  } while (0)

#define print_op1_builtin(prec, op, x, ns)                   \
  do {                                                       \
    __typeof(x) res = __builtin_ ## op(x);                   \
    sprint_fp(sprint_buf, "%." #prec "f", res, ns);          \
    printf("%s (builtin): %s\n", #op, sprint_buf);           \
  } while (0)

#define print_op1(prec, op, x, ns)                           \
  print_op1_libm(prec, op, x, ns);                           \
  print_op1_builtin(prec, op, x, ns);

#define print_op1_llvm(prec, op, x, ns)                      \
  do {                                                       \
    __typeof(x) res = llvm_intrinsic_ ## op(x);              \
    sprint_fp(sprint_buf, "%." #prec "f", res, ns);          \
    printf("%s (llvm): %s\n", #op, sprint_buf);              \
  } while (0)

#define print_op2(prec, op, x, y, ns)                        \
  do {                                                       \
    __typeof(x) res = op(x, y);                              \
    sprint_fp(sprint_buf, "%f", y, ns);                      \
    printf("%s(%f, %s) (math.h): ", #op, x, sprint_buf);     \
    sprint_fp(sprint_buf, "%." #prec "f", res, ns);          \
    printf("%s\n", sprint_buf);                              \
    res = __builtin_ ## op(x, y);                            \
    sprint_fp(sprint_buf, "%f", y, ns);                      \
    printf("%s(%f, %s) (builtin): ", #op, x, sprint_buf);    \
    sprint_fp(sprint_buf, "%." #prec "f", res, ns);          \
    printf("%s\n", sprint_buf);                              \
  } while (0)

#define print_vec_op_llvm(prec, op, x, ns)                   \
  do {                                                       \
    __typeof(x) res = llvm_intrinsic_ ## op(x);              \
    sprint_v4f32(sprint_buf, "%." #prec "f", res, ns);       \
    printf("%s (llvm): %s\n", #op, sprint_buf);              \
  } while (0)

int main(int argc, char* argv[]) {
  char sprint_buf[512];
  /*
   * Use no_nan_sign when the sign bit of a NaN is not consistent,
   * and just print a positive nan always in that case to get the
   * test to pass.
   */
  int no_nan_sign = 0;
  int nan_sign = 1;

  for (int i = 0; i < ARRAY_SIZE_UNSAFE(f32); ++i) {
    sprint_fp(sprint_buf, "%.6f", f32[i], nan_sign);
    printf("\nf32 value is: %s\n", sprint_buf);
    /*
     * We may want to fix this to have a consistent nan sign bit.
     * On x86, the llvm.sqrt intrinsic returns -nan for negative values
     * while the le32 libm/builtin always returns plain nan.
     *
     * On ARM, the llvm.sqrt intrinsic is consistent with the
     * libm/builtin function.
     *
     * However, on x86_64-nacl-clang, the builtin is the same as the intrinsic
     * and different from libm. Also, with -ffast-math, the libm function
     * call is converted to the intrinsic.
     *
     * So, the only consistent one is libm, and for the rest we conservatively
     * disable checking the sign bit.
     *
     * NOTE: change no_nan_sign to nan_sign to test.
     * https://code.google.com/p/nativeclient/issues/detail?id=4038
     */
    print_op1_libm(5, sqrtf, f32[i], no_nan_sign);
    print_op1_builtin(5, sqrtf, f32[i], no_nan_sign);
    print_op1_llvm(5, sqrtf, f32[i], no_nan_sign);
    /*
     * logf(-snan) produces "-qnan" on X86/ARM, yet it can produce "+qnan"
     * on MIPS32 platforms. Thus, we can not rely on sign of it for log/exp.
     */
    print_op1(5, logf, f32[i], no_nan_sign);
    print_op1(5, log2f, f32[i], no_nan_sign);
    print_op1(5, log10f, f32[i], no_nan_sign);
    print_op1(4, expf, f32[i], no_nan_sign);
    print_op1(4, exp2f, f32[i], nan_sign);
    /* We may want to fix this to be consistent re: nan sign bit.
     * On x86, sin/cos of inf/-inf the functions give -nan,
     * while on ARM it is nan.
     * https://code.google.com/p/nativeclient/issues/detail?id=4040
     */
    print_op1(5, sinf, f32[i], no_nan_sign);
    print_op1(5, cosf, f32[i], no_nan_sign);
    print_op2(5, powf, neg_base32, f32[i], nan_sign);
    print_op2(5, powf, base32, f32[i], nan_sign);
    print_op1(5, fabsf, f32[i], nan_sign);
    print_op1_llvm(5, fabsf, f32[i], nan_sign);
  }

  for (int i = 0; i < ARRAY_SIZE_UNSAFE(f64); ++i) {
    sprint_fp(sprint_buf, "%.6f", f64[i], nan_sign);
    printf("\nf64 value is: %s\n", sprint_buf);
    print_op1_libm(6, sqrt, f64[i], no_nan_sign);
    print_op1_builtin(6, sqrt, f64[i], no_nan_sign);
    print_op1_llvm(6, sqrt, f64[i], no_nan_sign);
    /*
     * See the comment above: log(-snan) produces result of different sign
     * on X86/ARM platforms on one side, and MIPS32 on the other.
     */
    print_op1(6, log, f64[i], no_nan_sign);
    print_op1(6, log2, f64[i], no_nan_sign);
    print_op1(6, log10, f64[i], no_nan_sign);
    print_op1(6, exp, f64[i], no_nan_sign);
    print_op1(6, exp2, f64[i], nan_sign);
    print_op1(6, sin, f64[i], no_nan_sign);
    print_op1(6, cos, f64[i], no_nan_sign);
    print_op2(6, pow, neg_base64, f64[i], nan_sign);
    print_op2(6, pow, base64, f64[i], nan_sign);
    print_op1(6, fabs, f64[i], nan_sign);
    print_op1_llvm(6, fabs, f64[i], nan_sign);
  }

  /* Test vectors on the intrinsic with a few samples (no libm equiv). */
  printf("\nTesting vectors (forward samples)\n");
  v4f32 test_vec;
  int num_elems = sizeof(test_vec) / sizeof(test_vec[0]);
  for (int i = 0; i + (num_elems - 1) < ARRAY_SIZE_UNSAFE(f32); i += 2) {
    for (int j = 0; j < num_elems; ++j) {
      test_vec[j] = f32[i + j];
    }
    sprint_v4f32(sprint_buf, "%.6f", test_vec, nan_sign);
    printf("vec value is: %s\n", sprint_buf);
    print_vec_op_llvm(6, vec_fabs, test_vec, nan_sign);
  }

  printf("\nTesting vectors (pseudo-random samples)\n");
  srand48(1234);
  int num_samples = 16;
  for (int i = 0; i < num_samples; ++i) {
    for (int j = 0; j < num_elems; ++j) {
      test_vec[j] = f32[lrand48() % ARRAY_SIZE_UNSAFE(f32)];
    }
    sprint_v4f32(sprint_buf, "%.6f", test_vec, nan_sign);
    printf("vec value is: %s\n", sprint_buf);
    print_vec_op_llvm(6, vec_fabs, test_vec, nan_sign);
  }

  return 0;
}
