/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <emmintrin.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* The type __m128 invokes use of the x86-specific SSE registers.
   Some loads/stores of these, such as movaps, require 16-byte memory
   alignment, and fault on unaligned addresses.

   gcc normally assumes the stack is 16-byte-aligned, so in principle
   it should generate movaps instructions for stack variables.  That
   would mean that if the runtime failed to align the stack properly,
   the code below would fault.

   It turns out that gcc 4.4 does not make use of the stack's
   alignment and falls back to a movlps/movhps pair, which do not
   require alignment.  Though I have not seen this test fail, we keep
   it for future versions of gcc and because we do not have any SSE
   tests otherwise. */

__m128 v1;
__m128 v2;
__m128 *dummy;

/* Prevent a variable's stack allocation from being optimised away by
   assigning its address to a global variable, which could be read by
   another compilation unit.  Calling a function in another
   compilation unit should also prevent the variable assignments below
   from being optimised away. */
void ForceStackAllocation(__m128 *ptr) {
  dummy = ptr;
  printf("Variable address: %p\n", (void *) ptr);
}

void *ThreadFunc(void *arg) {
  __m128 var;

  printf("Check alignment in second thread...\n");
  var = v1;
  ForceStackAllocation(&var);
  v2 = var;
  ForceStackAllocation(&var);

  return NULL;
}

int main(void) {
  pthread_t tid;
  int err;

  __m128 var;

  /* Turn off stdout buffering to aid debugging in case of a crash. */
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("Check alignment in initial thread...\n");
  var = v1;
  ForceStackAllocation(&var);
  v2 = var;
  ForceStackAllocation(&var);

  err = pthread_create(&tid, NULL, ThreadFunc, NULL);
  assert(err == 0);
  err = pthread_join(tid, NULL);
  assert(err == 0);

  return 0;
}
