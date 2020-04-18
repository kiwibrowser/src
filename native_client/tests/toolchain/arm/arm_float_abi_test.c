/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test that the ARM float ABI we use for nexes is "hard", not soft.
 *
 * We call some undefined functions (extern), compile this to a .o
 * and disassemble it to check for key assembly instructions.
 *
 * This test uses LLVM's FileCheck utility. For more information,
 * please refer to http://llvm.org/docs/CommandGuide/FileCheck.html
 */


/* Calling. */

extern void pass_floats(float x);

void call_floats(float a, float b) {
  // CHECK-LABEL: call_floats
  // CHECK-NEXT: {{.*}}vmov.f32{{.*}}s0,{{.*}}s1
  pass_floats(b);
}

extern void pass_doubles(double x);

void call_doubles(double a, double b) {
  // CHECK-LABEL: call_doubles
  // CHECK-NEXT: {{.*}}vmov.f64{{.*}}d0,{{.*}}d1
  pass_doubles(b);
}

extern void pass_mixed(int x, double y);

void call_mixed(int a, float b, double c, char d) {
  // CHECK-LABEL: call_mixed
  // CHECK-NEXT: {{.*}}vmov.f64{{.*}}d0,{{.*}}d1
  pass_mixed(a, c);
}

/* Check homogenous aggregates vs non-homogeneous aggregates. */
typedef struct {
  float x;
  float y;
  float z;
  float w;
} homogeneous_agg_t __attribute__((aligned(16)));

typedef union {
  float f;
  int i;
} float_int_u;

typedef struct {
  float_int_u x;
  float y;
  float z;
  float w;
} non_homogeneous_agg_t __attribute__((aligned(16)));

extern void pass_homogenous(homogeneous_agg_t s);

void call_homogeneous(homogeneous_agg_t s, homogeneous_agg_t s2) {
  /* PNaCl bitcode ABI uses "byval" which means that these are currently
   * passed on the stack, regardless of float ABI.  Not checking for now.
   */
  pass_homogenous(s2);
}

extern void pass_non_homogenous(non_homogeneous_agg_t s);

void call_non_homogeneous(non_homogeneous_agg_t s,
                          non_homogeneous_agg_t s2) {
  /* PNaCl bitcode ABI uses "byval" which means that these are currently
   * passed on the stack, regardless of float ABI.
   * We don't have the bitcode metadata to know about unions anyway.
   * Not checking for now.
   */
  pass_non_homogenous(s2);
}

/* Returning. */

float return_float(float a, float b) {
  // CHECK-LABEL: return_float
  // CHECK-NEXT: {{.*}}vmov.f32{{.*}}s0,{{.*}}s1
  return b;
}

double return_double(double a, double b) {
  // CHECK-LABEL: return_double
  // CHECK-NEXT: {{.*}}vmov.f64{{.*}}d0,{{.*}}d1
  return b;
}
