/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/toolchain/eh_helper.h"

#include <assert.h>
#include <math.h>

// Test that EH stack unwinding works when callee-saved floating point
// registers may be saved (e.g., on ARM).
// Also test that the callee-saved registers are restored during
// stack unwinding, for use in a catch-statement.

namespace {

// Make a float x 4, without including arch-specific headers.
typedef float v4sf __attribute__ ((__vector_size__(16)));

// Make it a union to help w/ accessing an individual element.
typedef union {
  float elems[4];
  v4sf v;
} Vec_4_f;

// Make some volatile constants to trick the optimizer.
volatile double kZero = 0.0;
volatile double kOne = 1.0;
volatile double kPiOver2 = M_PI_2;
volatile double kE = M_E;
volatile Vec_4_f kVOnes __attribute__((aligned(16))) = {
  { (float)kOne, (float)kOne, (float)kOne, (float)kOne }
};

// Make a function with roughly this structure:
// x, y, z = Heavy-Floating-Point-Compute;
// call();
// use(x, y, z);
//
// That makes it likely that x, y, z use callee-saved registers, instead of
// restoring values from the stack after the call.
void __attribute__((noinline)) outer(
    double x, double y, double z, double w);
void outer(double x, double y, double z, double w) {
  // Assume that the caller gives us appropriate values so that these
  // computations give 0 as the result.
  double should_be_zero1 = round(sqrt(x));
  double should_be_zero2 = round(cos(y));
  double should_be_zero3 = round(sin(z));
  double should_be_zero4 = round(log(w));
  double should_be_zero;
  Vec_4_f v_should_be_ones  __attribute__((aligned(16))),
      v_will_be_zeros  __attribute__((aligned(16)));
  v_should_be_ones.v = kVOnes.v;
  // Start v_will_be_zeros as all ones.
  // Then do v_will_be_zeros = <1...> * <1...> - <1...>.
  v_will_be_zeros.v = kVOnes.v;
  v_will_be_zeros.v =
      v_will_be_zeros.v * v_should_be_ones.v - v_should_be_ones.v;
  // Function call to make it worth using callee-saved regs.
  printf("Should be zero: %f, %f, %f, %f, %f\n",
         should_be_zero1, should_be_zero2, should_be_zero3, should_be_zero4,
         v_will_be_zeros.elems[0]);
  if (should_be_zero1 > should_be_zero2)
    should_be_zero = should_be_zero1;
  else if (should_be_zero2 > should_be_zero3)
    should_be_zero = should_be_zero2;
  else if (should_be_zero3 > should_be_zero4)
    should_be_zero = should_be_zero3;
  else
    should_be_zero = should_be_zero4;
  next_step(static_cast<int>(3 - should_be_zero));
  if (v_will_be_zeros.elems[0] > v_will_be_zeros.elems[1])
    throw static_cast<int>(v_will_be_zeros.elems[0]);
  else if (v_will_be_zeros.elems[1] > v_will_be_zeros.elems[2])
    throw static_cast<int>(v_will_be_zeros.elems[1]);
  else if (v_will_be_zeros.elems[2] > v_will_be_zeros.elems[3])
    throw static_cast<int>(v_will_be_zeros.elems[2]);
  else
    throw static_cast<int>(v_will_be_zeros.elems[3]);
}

}  // namespace

class B {
 public:
  explicit B(double should_be_one) {
    next_step(static_cast<int>(5 * should_be_one));
  }
  ~B() { next_step(7); }
};

void __attribute__((noinline)) middle(int never_55);
void middle(int never_55) {
  double should_be_one1 = sqrt(kOne) * cos(kZero);
  double should_be_one2 = sin(kPiOver2) * log(kE);
  Vec_4_f v_should_be_ones1 __attribute__((aligned(16))),
      v_should_be_ones2 __attribute__((aligned(16)));
  v_should_be_ones1.v = kVOnes.v;
  v_should_be_ones2.v = kVOnes.v;
  v_should_be_ones1.v = v_should_be_ones1.v * v_should_be_ones2.v;
  printf("Should be one: %f, %f, %f\n",
         should_be_one1, should_be_one2, v_should_be_ones1.elems[0]);
  try {
    // Function call to make it worth using callee-saved regs.
    outer(kZero, kPiOver2, kZero, kOne);
  } catch(int x) {
    if (x != 0)
      abort();
    printf("Should still be one: %f, %f, %f\n",
           should_be_one1, should_be_one2, v_should_be_ones1.elems[0]);
    next_step(4);
    if (should_be_one1 > should_be_one2) {
      throw B(should_be_one1);
    } else if (should_be_one2 > v_should_be_ones1.elems[0]) {
      throw B(should_be_one2);
    } else {
      throw B(v_should_be_ones1.elems[0]);
    }
  }
  abort();
}

int main(int argc, char* argv[]) {
  double should_be_e = kE;
  double should_be_pi_2 = kPiOver2;
  double should_be_63 = sqrt(kOne) * cos(kZero) * 63;
  double should_be_127 = sin(kPiOver2) * log(kE) * 127;
  next_step(1);
  try {
    next_step(2);
    middle(argc);
  } catch(...) {
    fprintf(stderr, "should_be_e=%f, should_be_pi_2=%f, "
            "should_be_63=%f, shouldbe_127=%f\n",
            should_be_e, should_be_pi_2, should_be_63, should_be_127);
    assert(should_be_e == kE);
    assert(should_be_pi_2 == kPiOver2);
    assert(should_be_63 == 63.0);
    assert(should_be_127 == 127.0);
    next_step(6);
  }
  next_step(8);
  return 55;
}
