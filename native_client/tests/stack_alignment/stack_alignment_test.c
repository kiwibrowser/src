/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>


/* On x86, gcc normally assumes that the stack is already
   16-byte-aligned.  It is the responsibility of the runtime to ensure
   that stacks are set up this way.  This test checks that the runtime
   does this properly. */

struct AlignedType {
  int blah;
} __attribute__((aligned(16)));

/* We do this check in a separate function just in case the compiler
   is clever enough to optimise away the expression
     (uintptr_t) &var % 16 == 0
   for a stack-allocated variable. */
void CheckAlignment(void *pointer) {
  printf("Variable address: %p\n", pointer);
  assert((uintptr_t) pointer % 16 == 0);
}

void *ThreadFunc(void *arg) {
  struct AlignedType var;
  printf("Check alignment in second thread...\n");
  CheckAlignment(&var);
  return NULL;
}

int main(void) {
  pthread_t tid;
  int err;

  struct AlignedType var;

  /* Turn off stdout buffering to aid debugging in case of a crash. */
  setvbuf(stdout, NULL, _IONBF, 0);

  printf("Check alignment in initial thread...\n");
  CheckAlignment(&var);

  err = pthread_create(&tid, NULL, ThreadFunc, NULL);
  assert(err == 0);
  err = pthread_join(tid, NULL);
  assert(err == 0);

  return 0;
}
