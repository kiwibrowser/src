/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_semaphore.h"

#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/platform_init.h"

#include "native_client/src/trusted/service_runtime/include/sys/time.h"

#define STACK_SIZE_BYTES        (4 * 4096)
#define NUM_TRIES_SUFFICIENT    (5)

void ThreadSleepMs(uint64_t msec) {
  struct nacl_abi_timespec nap_duration;

  nap_duration.tv_sec = (nacl_abi_time_t) (msec / 1000);
  nap_duration.tv_nsec = (long) (msec % 1000);
  NaClNanosleep(&nap_duration, (struct nacl_abi_timespec *) NULL);
}

void PauseSpinningThread(void) {
  ThreadSleepMs(10);
}

size_t gNumTriesSufficient = NUM_TRIES_SUFFICIENT;
struct NaClSemaphore gSem;
struct NaClMutex gMu;
struct NaClCondVar gCv;
size_t gNumThreadsDone = 0;
size_t gNumThreadsTried = 0;
int gFailure = 0;

void WINAPI ThreadMain(void *personality) {
  int       thread_num = (int) (uintptr_t) personality;
  uint64_t  sleep_count;
  int       got_sem = 0;
  int       failed = 0;

  for (sleep_count = 0; sleep_count < gNumTriesSufficient; ++sleep_count) {
    /* the sem_trywait should not succeed the first time through */
    if (NACL_SYNC_BUSY != NaClSemTryWait(&gSem)) {
      got_sem = 1;
      break;
    }
    if (0 == sleep_count) {
      NaClXMutexLock(&gMu);
      ++gNumThreadsTried;
      NaClXCondVarSignal(&gCv);
      NaClXMutexUnlock(&gMu);
    }
    PauseSpinningThread();
  }

  if (got_sem) {
    printf("Thread %d: NaClSemTryWait succeeded at %"NACL_PRId64"\n",
           thread_num,
           sleep_count);
  } else {
    /* gNumThreadsTried == sleep_count */
    printf("Thread %d: NaClSemWait\n", thread_num);
    if (NACL_SYNC_OK != NaClSemWait(&gSem)) {
      printf("FAILED\n");
      printf("NaClSemWait failed!?!\n");
      failed = 1;
    }
  }

  if (0 == sleep_count) {
    printf("FAILED\n");
    printf("Thread %d never actually waited at NaClSemTryWait\n", thread_num);
    failed = 1;
  } else {
    printf("OK -- thread %d\n", thread_num);
  }

  NaClXMutexLock(&gMu);
  gFailure += failed;
  ++gNumThreadsDone;
  NaClXCondVarSignal(&gCv);
  NaClXMutexUnlock(&gMu);
}

int main(int ac, char **av) {
  int exit_status = -1;
  int opt;
  size_t num_threads = 16;
  size_t n;
  struct NaClThread thr;

  while (EOF != (opt = getopt(ac, av, "n:s:t:"))) {
    switch (opt) {
      case 'n':
        num_threads = strtoul(optarg, (char **) NULL, 0);
        break;
      case 't':
        gNumTriesSufficient = strtoul(optarg, (char **) NULL, 0);
        break;
      default:
        fprintf(stderr,
                "Usage: nacl_semaphore_test [args]\n"
                "  -n n   number of threads used to test semaphore\n"
                "  -t n   number of TryWait operations before blocking Try\n");
        goto cleanup0;
    }
  }

  NaClPlatformInit();

  if (!NaClSemCtor(&gSem, 0)) {
    fprintf(stderr, "nacl_semaphore_test: NaClSemCtor failed!\n");
    goto cleanup1;
  }
  if (!NaClMutexCtor(&gMu)) {
    fprintf(stderr, "nacl_semaphore_test: NaClMutexCtor failed!\n");
    goto cleanup2;
  }
  if (!NaClCondVarCtor(&gCv)) {
    fprintf(stderr, "nacl_semaphore_test: NaClCondVarCtor failed!\n");
    goto cleanup3;
  }

  for (n = 0; n < num_threads; ++n) {
    if (!NaClThreadCtor(&thr, ThreadMain, (void *) (uintptr_t) n,
                        STACK_SIZE_BYTES)) {
      fprintf(stderr,
              "nacl_semaphore_test: could not create thread %"NACL_PRIdS"\n",
              n);
      goto cleanup4;  /* osx leak semaphore otherwise */
    }
  }

  NaClXMutexLock(&gMu);
  while (gNumThreadsTried != num_threads) {
    NaClXCondVarWait(&gCv, &gMu);
  }
  NaClXMutexUnlock(&gMu);

  for (n = 0; n < num_threads; ++n) {
    NaClSemPost(&gSem);  /* let a thread go */
  }

  NaClXMutexLock(&gMu);
  while (gNumThreadsDone != num_threads) {
    NaClXCondVarWait(&gCv, &gMu);
  }
  exit_status = gFailure;
  NaClXMutexUnlock(&gMu);

  if (0 == exit_status) {
    printf("SUCCESS\n");
  }
 cleanup4:
  /* single exit with (ah hem) simulation of RAII via cleanup sled */
  NaClCondVarDtor(&gCv);
 cleanup3:
  NaClMutexDtor(&gMu);
 cleanup2:
  NaClSemDtor(&gSem);
 cleanup1:
  NaClPlatformFini();
 cleanup0:
  return exit_status;
}
