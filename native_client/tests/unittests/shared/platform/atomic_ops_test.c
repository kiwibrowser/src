/* Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/atomic_ops.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_sync.h"


#define THREAD_STACK_SIZE  (128*1024)

int32_t gIncrementsPerThread = 1000;
int32_t gNumThreads;

struct NaClMutex gMutex;


/* For atomic counter test */
Atomic32 gCounter = 0;

static void IncrementTest(void) {
  /* Increment the counter, gIncrementsPerThread times,
   * with the values 1...gIncrementsPerThread
   */
  Atomic32 i;
  for (i=1; i <= gIncrementsPerThread; i++) {
    AtomicIncrement(&gCounter, i);
  }
}

static void DecrementTest(void) {
  /* Decrement the counter gIncrementsPerThread times,
   * with the values 1...gIncrementsPerThread-1
   */
  Atomic32 i;
  for (i=1; i < gIncrementsPerThread; i++) {
    AtomicIncrement(&gCounter, -i);
  }
}

/* Atomic exchange test
 * Each thread exchanges gExchange for its tid.
 * It takes the returned value and adds it to gExchangeSum.
 * When finished, gExchangeSum + gExchange will contain
 * 1 + ... + gNumThreads
 */
Atomic32 gExchange = 0;
Atomic32 gExchangeSum = 0;

static void ExchangeTest(int32_t tid) {
  Atomic32 i;
  i = AtomicExchange(&gExchange, tid);
  if (i == tid) {
    fprintf(stderr,
            "Error: AtomicExchange returned the new value instead of old.\n");
    exit(EXIT_FAILURE);
  }
  AtomicIncrement(&gExchangeSum, i);
}

/*
 * Atomic compare and swap test.
 *
 * Each thread spins until gSwap == tid, and then exchanges it for tid + 1.
 *
 * If the threads are scheduled in the order they are launched, the
 * CompareAndSwap() loops will complete quickly.  However, if they are
 * scheduled out-of-order, these loops could take a long time to
 * complete.  Yielding the CPU here improves that significantly.
 */
Atomic32 gSwap = 1;
static void CompareAndSwapTest(int32_t tid) {
  while (CompareAndSwap(&gSwap, tid, tid + 1) != tid) {
    NaClThreadYield();
  }
}

void WINAPI ThreadMain(void *state) {
  int32_t tid = (int32_t) (intptr_t) state;

  /* Wait for the signal to begin */
  NaClXMutexLock(&gMutex);
  NaClXMutexUnlock(&gMutex);

  /* Swap the order to shake things up a bit */
  if (tid % 2 == 0) {
    IncrementTest();
    DecrementTest();
  } else {
    DecrementTest();
    IncrementTest();
  }

  ExchangeTest(tid);

  CompareAndSwapTest(tid);
}

int main(int argc, const char *argv[]) {
  struct NaClThread *threads;
  int rv;
  int32_t tid;
  int32_t tmp;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <NumThreads>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  gNumThreads = strtol(argv[1], NULL, 10);

  threads = (struct NaClThread*)malloc(gNumThreads*sizeof(struct NaClThread));
  if (threads == NULL) {
    fprintf(stderr, "malloc returned NULL\n");
    exit(EXIT_FAILURE);
  }

  if (!NaClMutexCtor(&gMutex)) {
    fprintf(stderr, "NaClMutexCtor failed\n");
    exit(EXIT_FAILURE);
  }

  NaClXMutexLock(&gMutex);

  for (tid = 1; tid <= gNumThreads; ++tid) {
    fprintf(stderr, "Creating thread %d\n", (int)tid);

    rv = NaClThreadCreateJoinable(&threads[tid-1],
                                  ThreadMain,
                                  (void*) (intptr_t) tid,
                                  THREAD_STACK_SIZE);
    if (!rv) {
      fprintf(stderr, "NaClThreadCtor failed\n");
      exit(EXIT_FAILURE);
    }
  }

  NaClXMutexUnlock(&gMutex);

  for (tid = 1; tid <= gNumThreads; ++tid) {
    NaClThreadJoin(&threads[tid-1]);
  }

  /* Check the results */
  tmp = gIncrementsPerThread * gNumThreads;
  if (gCounter != tmp) {
    fprintf(stderr, "ERROR: gCounter is wrong. Expected %d, got %d\n",
           (int)tmp, (int)gCounter);
    exit(EXIT_FAILURE);
  }

  tmp = gNumThreads*(gNumThreads+1)/2;
  if (gExchange + gExchangeSum != tmp) {
    fprintf(stderr,
            "ERROR: gExchange+gExchangeSum is wrong. Expected %d, got %d\n",
            (int)tmp, (int)(gExchange + gExchangeSum));
    exit(EXIT_FAILURE);
  }

  if (gSwap != gNumThreads+1) {
    fprintf(stderr, "ERROR: gSwap is wrong. Expected %d, got %d\n",
            (int)(gNumThreads+1), (int)gSwap);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "PASSED\n");
  NaClMutexDtor(&gMutex);
  free(threads);
  return 0;
}
