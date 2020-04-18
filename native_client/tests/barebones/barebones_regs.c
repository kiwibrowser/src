/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl test for super simple program not using newlib
 * Try to use as many regs as possible
 * use gcc -E <src.c> to see what all this macro magic does
 *
 * For slower architectures (e.g. ARM) we provide a faster version
 * which can be selectedted via -DSMALL_REGS_TEST
 */
#if defined(SMALL_REGS_TEST)
#define REPEAT    \
  REPEAT_BODY(00) \
  REPEAT_BODY(01) \
  REPEAT_BODY(02) \
  REPEAT_BODY(03) \
  REPEAT_BODY(04) \
  REPEAT_BODY(05) \
  REPEAT_BODY(06) \
  REPEAT_BODY(07) \
  REPEAT_BODY(08) \
  REPEAT_BODY(09) \
  REPEAT_BODY(10)
#else
#define REPEAT    \
  REPEAT_BODY(00) \
  REPEAT_BODY(01) \
  REPEAT_BODY(02) \
  REPEAT_BODY(03) \
  REPEAT_BODY(04) \
  REPEAT_BODY(05) \
  REPEAT_BODY(06) \
  REPEAT_BODY(07) \
  REPEAT_BODY(08) \
  REPEAT_BODY(09) \
  REPEAT_BODY(10) \
  REPEAT_BODY(11) \
  REPEAT_BODY(12) \
  REPEAT_BODY(13) \
  REPEAT_BODY(14) \
  REPEAT_BODY(15) \
  REPEAT_BODY(16) \
  REPEAT_BODY(17) \
  REPEAT_BODY(18)
#endif

int dummy = 666;
int sum = 55;

volatile int STRIDE = 8;

#include "barebones.h"

/* declare some arrays used by the loops */
#undef REPEAT_BODY
#define REPEAT_BODY(N) int array ## N [20];
REPEAT


int main(void) {
  /* declare loop variables */
#undef REPEAT_BODY
#define REPEAT_BODY(N) int i ## N;
REPEAT

  /* declare loop variables */
#undef REPEAT_BODY
#define REPEAT_BODY(N) for( i ## N = 0 ;  i ## N < 20; i ## N++) \
    array ## N [ i ## N ] =  i ## N;
REPEAT

  /* now loop */
#undef REPEAT_BODY
#define REPEAT_BODY(N) for( i ## N = 0;  i ## N < 20; i ## N += STRIDE)
REPEAT
  {
    /* NOTE: one of the factors is zero, hece we are always adding zero */
    sum += 1
#undef REPEAT_BODY
#define REPEAT_BODY(N) * array ## N [i ## N]
REPEAT
           ;
  }
  NACL_SYSCALL(exit)(sum);
  /* UNREACHABLE */
  return 0;
}
