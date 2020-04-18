/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void sincos(double a, double *s, double *c);
void sincosf(float a, float *s, float *c);

int TestSinCosD(double a) {
  const double maxerr = 0.000000000001;
  double sincos_sin, sincos_cos, sin_sin, cos_cos;

  sincos(a, &sincos_sin, &sincos_cos);
  sin_sin = sin(a);
  cos_cos = cos(a);
  if (fabs(sincos_sin - sin_sin) > maxerr ||
      fabs(sincos_cos - cos_cos) > maxerr) {
    printf("sincosf(%12.12f) outside tolerance: sin:%12.12f, cos:%12.12f\n",
            a, sincos_sin - sin_sin, sincos_cos - cos_cos);
    return 1;
  } else {
    return 0;
  }
}

int TestSinCosF(float a) {
  const float maxerr = 0.000000000001;
  float sincos_sin, sincos_cos, sin_sin, cos_cos;

  sincosf(a, &sincos_sin, &sincos_cos);
  sin_sin = sinf(a);
  cos_cos = cosf(a);
  if (fabsf(sincos_sin - sin_sin) > maxerr ||
      fabsf(sincos_cos - cos_cos) > maxerr) {
    printf("sincosf(%12.12f) outside tolerance: sin:%12.12f, cos:%12.12f\n",
            a, sincos_sin - sin_sin, sincos_cos - cos_cos);
    return 1;
  } else {
    return 0;
  }
}

int main(int argc, char* argv[]) {
  const double pi = 3.14159265;
  const double fourpi = 4.0 * pi;
  double a;
  int i;
  int sawproblem = 0;

  for (a = -fourpi; a < fourpi; a += pi / 8.0) {
    sawproblem |= TestSinCosD(a);
    sawproblem |= TestSinCosF(a);
  }
  /* seed drand48() generator to make it deterministic. */
  srand48(12345678);
  for (i = 0; i < 100; i++) {
    sawproblem |= TestSinCosD(fourpi * (drand48() - 0.5));
    sawproblem |= TestSinCosF(drand48());
  }
  if (!sawproblem) {
    printf("No problems!\n");
  }
  return 0;
}
