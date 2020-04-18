/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test ensures that PNaCl can deal with ``long double``, and that
 * the C++11 standard library functions work.
 */

#include <cassert>
#include <cmath>
#include <stdio.h>

volatile long double ld = 3.14159265359;
long double ld2;
volatile double d = 3.14159265359;
volatile float f = 3.14159265359;
volatile long l = 42;
volatile int i = 42;
int i2;

int main() {
  // PNaCl represents ``long double`` as a 64-bit value.
  assert(sizeof(long double) == sizeof(double));

  printf("%Lf\n", ld);

  // All of the <cmath> functions that involve ``long double``.
  printf("%Lf\n",  std::acos(ld));
  printf("%Lf\n",  std::asin(ld));
  printf("%Lf\n",  std::atan(ld));
  printf("%Lf\n",  std::atan2(ld, ld));
  printf("%Lf\n",  std::ceil(ld));
  printf("%Lf\n",  std::cos(ld));
  printf("%Lf\n",  std::cosh(ld));
  printf("%Lf\n",  std::exp(ld));
  printf("%Lf\n",  std::fabs(ld));
  printf("%Lf\n",  std::floor(ld));
  printf("%Lf\n",  std::fmod(ld, ld));
  printf("%Lf\n",  std::frexp(ld, &i2));
  printf("%Lf\n",  std::ldexp(ld, i));
  printf("%Lf\n",  std::log(ld));
  printf("%Lf\n",  std::log10(ld));
  printf("%Lf\n",  std::modf(ld, &ld2));
  printf("%Lf\n",  std::pow(ld, ld));
  printf("%Lf\n",  std::sin(ld));
  printf("%Lf\n",  std::sinh(ld));
  printf("%Lf\n",  std::sqrt(ld));
  printf("%Lf\n",  std::tan(ld));
  printf("%Lf\n",  std::tanh(ld));
  printf("%Lf\n",  std::acosh(ld));
  printf("%Lf\n",  std::asinh(ld));
  printf("%Lf\n",  std::atanh(ld));
  printf("%Lf\n",  std::cbrt(ld));
  printf("%Lf\n",  std::copysign(ld, ld));
  printf("%Lf\n",  std::erf(ld));
  printf("%Lf\n",  std::erfc(ld));
  printf("%Lf\n",  std::exp2(ld));
  printf("%Lf\n",  std::expm1(ld));
  printf("%Lf\n",  std::fdim(ld, ld));
  /*
   * TODO(jfb) FMA is currently disallowed by PNaCl's ABI.
   * printf("%Lf\n",  std::fma(ld, ld, ld));
   */
  printf("%Lf\n",  std::fmax(ld, ld));
  printf("%Lf\n",  std::fmin(ld, ld));
  printf("%Lf\n",  std::hypot(ld, ld));
  printf("%i\n",   std::ilogb(ld));
  printf("%Lf\n",  std::lgamma(ld));
  printf("%lli\n", std::llrint(ld));
  printf("%lli\n", std::llround(ld));
  printf("%Lf\n",  std::log1p(ld));
  printf("%Lf\n",  std::log2(ld));
  printf("%Lf\n",  std::log2(ld));
  printf("%Lf\n",  std::logb(ld));
  printf("%Lf\n",  std::logb(ld));
  printf("%li\n",  std::lrint(ld));
  printf("%li\n",  std::lround(ld));
  printf("%Lf\n",  std::nearbyint(ld));
  printf("%Lf\n",  std::nextafter(ld, ld));
  /*
   * TODO(jfb) Currently unsupported by LLVM.
   * printf("%f\n",   std::nexttoward(f, ld));
   * printf("%Lf\n",  std::nexttoward(ld, ld));
   * printf("%f\n",   std::nexttoward<int>(i, ld));
  */
  printf("%Lf\n",  std::remainder(ld, ld));
  printf("%Lf\n",  std::remquo(ld, ld, &i2));
  printf("%Lf\n",  std::rint(ld));
  printf("%Lf\n",  std::round(ld));
  printf("%Lf\n",  std::scalbln(ld, l));
  printf("%Lf\n",  std::scalbn(ld, i));
  printf("%Lf\n",  std::tgamma(ld));
  printf("%Lf\n",  std::trunc(ld));

  return 0;
}
