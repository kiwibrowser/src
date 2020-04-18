/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WSIZE sizeof(uint32_t)

/*
 * Generate a deterministic sequence of numbers.
 * Use the Hailstone sequence, just for fun.
 */
uint32_t next_seq(uint32_t num) {
  if (num & 1) {
    return 3 * num + 1;
  } else {
    return num / 2;
  }
}

/*
 * Initialize the memory with a deterministic sequence. Given numwords = N,
 * the initialized memory will look like this (words):
 * [N, s1, s2, s3 ...]
 * s1 <- (addr % 1000). s2 <- next_seq(s1). s3 <- next_seq(s2), etc...
 */
void init_memory(void *addr, uint32_t numwords) {
  assert(numwords >= 1);
  uint32_t *wordaddr = (uint32_t*)addr;
  wordaddr[0] = numwords;
  for (int i = 1; i < numwords; i++) {
    wordaddr[i] = (i == 1) ? ((uint32_t)addr % 1000)
                           : next_seq(wordaddr[i - 1]);
  }
}

/* Verify that memory is initialized as expected from init_memory */
void verify_memory(void *addr) {
  assert(addr);
  uint32_t *wordaddr = (uint32_t*)addr;
  uint32_t numwords = wordaddr[0];
  assert(numwords >= 1);
  for (int i = 1; i < numwords; i++) {
    if ((i == 1 && wordaddr[1] == ((uint32_t)addr % 1000)) ||
        wordaddr[i] == next_seq(wordaddr[i - 1])) {
      /* Good! */
    } else {
      printf("Memory verification error. Buffer starting at %p, index %u: %u\n",
             addr, i, wordaddr[i]);
      exit(1);
    }
  }
}

/*
 * Runs an allocation test using the specified allocator.
 */
typedef enum {
  USE_MALLOC, USE_CALLOC, USE_REALLOC
} WhichAlloc;

void run_allocation_test(WhichAlloc wa, uint32_t numallocs) {
  void **table = calloc(numallocs, sizeof(void*));
  for (int i = 0; i < numallocs; i++) {
    /* Random allocation size */
    uint32_t numwords = 1 + rand() % (2 << 15);

    /* Allocate the memory and save the pointer in table */
    void *addr = 0;
    if (wa == USE_MALLOC) {
      addr = malloc(WSIZE * numwords);
    } else if (wa == USE_CALLOC) {
      addr = calloc(numwords, WSIZE);
    } else if (wa == USE_REALLOC) {
      addr = realloc(0, WSIZE * numwords);
    }
    assert(addr);
    table[i] = addr;

    /* Initialize allocated memory */
    init_memory(addr, numwords);
  }

  /* To stir things up, allocate, init and release some memory */
#define NSTIR 10
  void *stir_table[NSTIR];
  for (int i = 0; i < NSTIR; ++i) {
    uint32_t numwords = 1 + rand() % (2 << 18);
    void *addr = malloc(WSIZE * numwords);
    init_memory(addr, numwords);
    stir_table[i] = addr;
  }
  for (int i = 0; i < NSTIR; ++i) {
    free(stir_table[i]);
  }

  /* When all was allocated, verify that all memory looks as expected */
  for (int i = 0; i < numallocs; i++) {
    verify_memory(table[i]);
  }

  /* Finally, free all allocated memory */
  for (int i = 0; i < numallocs; i++) {
    free(table[i]);
  }
  free(table);
}

/* A volatile pointer to thwart optimizations in some tests */
volatile void* volatile vaddr;

int main(int argc, char **argv) {
  unsigned seed = time(0);

  /* Allow overriding the seed from the command-line */
  if (argc > 1) {
    seed = atoi(argv[1]);
  }

  srand(seed);
  printf("Random seed = %u\n", seed);

  /*
   * Main allocation tests: loop-run run_allocation_test for the different ways
   * to allocate memory (with malloc, calloc and realloc).
   */
  const uint32_t NUMALLOCS = 20;
  const uint32_t NUMRUNS = 10;

  for (int i = 0; i < NUMRUNS; i++) {
    run_allocation_test(USE_MALLOC, NUMALLOCS);
    run_allocation_test(USE_CALLOC, NUMALLOCS);
    run_allocation_test(USE_REALLOC, NUMALLOCS);
  }

  /*
   * Test that calloc zeroes the memory properly.
   */
  uint32_t zero = 0;
  void *m = calloc(4, 1);
  assert(memcmp(m, &zero, 4) == 0);
  free(m);
  m = calloc(1, 1);
  assert(memcmp(m, &zero, 1) == 0);
  free(m);

  /*
   * Test that realloc does the right thing, leaving the old contents in place.
   */
  uint32_t nwords = 100;
  m = malloc(nwords * WSIZE);
  init_memory(m, nwords);

  /* Save a copy of m aside, to compare later. We can't just use verify_memory
   * on the reallocated buffer because its location can change.
   */
  void *m2 = malloc(nwords * WSIZE);
  memcpy(m2, m, nwords * WSIZE);

  m = realloc(m, 3 * nwords * WSIZE);
  assert(memcmp(m, m2, nwords * WSIZE) == 0);
  free(m);
  free(m2);

  /*
   * Testing corner cases. In many of these our only test is that nothing
   * crashes, but that's also something.
   */
  free(NULL);

  vaddr = malloc(0);
  free((void*)vaddr);

  vaddr = calloc(0, 4);
  free((void*)vaddr);

  vaddr = realloc(NULL, 0);
  free((void*)vaddr);

  vaddr = malloc(7);
  vaddr = realloc((void*)vaddr, 0);

  return 0;
}

