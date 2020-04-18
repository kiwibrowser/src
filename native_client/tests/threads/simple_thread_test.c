/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * simple test for NativeClient threads
 */

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

#define kNumThreads 2
#define kNumRounds 10
#define kHelloWorldString "hello world\n"

#ifdef __GLIBC__
/*
 * This is a non-standard typedef that nacl-newlib provides but
 * nacl-glibc does not provide.
 */
typedef int AtomicInt32;
#endif


/* tls */
__thread int tls_var_data_int = 666;
__thread char tls_var_data_string[100] = kHelloWorldString;


__thread int tls_var_bss_int;
__thread char tls_var_bss_string[100];

int GlobalCounter = 0;

AtomicInt32 GlobalError = 0;


void IncError(void) {
  __sync_fetch_and_add(&GlobalError, 1);
}


#define USE_PTHREAD_LIB
#if defined(USE_PTHREAD_LIB)

pthread_mutex_t mutex;


void Init(void) {
  pthread_mutex_init(&mutex, NULL);
}


void CriticalSectionEnter(void) {
  pthread_mutex_lock(&mutex);
}


void CriticalSectionLeave(void) {
  pthread_mutex_unlock(&mutex);
}

#else

AtomicWord GlobalMutex = 0;


void Init(void) {
}


void CriticalSectionEnter(void) {
  int old_value;
  do {
    old_value = AtomicExchange(&GlobalMutex, 1);
  } while (1 == old_value);
}


void CriticalSectionLeave(void) {
  AtomicExchange(&GlobalMutex, 0);
}

#endif


void CounterWait(int id, int round) {
  for (;;) {
    CriticalSectionEnter();
    if (round * kNumThreads + id == GlobalCounter) {
      CriticalSectionLeave();
      break;
    }
    CriticalSectionLeave();
    /* Give another thread a chance to do its work. */
    sched_yield();
  }
}


void CounterAdvance(int id, int round) {
  CriticalSectionEnter();
  GlobalCounter = round * kNumThreads + id + 1;
  CriticalSectionLeave();
  /* Give another thread a chance to do its work. */
  sched_yield();
}


void CheckAndUpdateTlsVars(int id, int round) {
  if (0 == round) {
    if (666 != tls_var_data_int ||
        0 != strcmp(tls_var_data_string, kHelloWorldString)) {
      printf("[%d] ERROR: bad initial tls data\n", id);
      IncError();
    }

    if ( 0 != tls_var_bss_int ||
        0 != strcmp(tls_var_bss_string, "")) {
      printf("[%d] ERROR: bad initial tls bss\n", id);
      IncError();
    }
  } else {
    if (round * kNumThreads + id != tls_var_data_int ||
        0 != strcmp(tls_var_data_string, kHelloWorldString)) {
      printf("[%d] ERROR: bad tls data\n", id);
      IncError();
    }

    if (round * kNumThreads + id != tls_var_bss_int ||
        0 != strcmp(tls_var_bss_string, kHelloWorldString)) {
      printf("[%d] ERROR: bad tls bss\n", id);
      IncError();
    }
  }

  /* prepare tls data for next round */
  tls_var_data_int = (round + 1) * kNumThreads + id;
  tls_var_bss_int = (round + 1) * kNumThreads + id;
  strcpy(tls_var_bss_string, tls_var_data_string);
}


void* MyThreadFunction(void* arg) {
  int id = *(int *)arg;
  int r;

  printf("[%d] entering thread\n", id);
  if (id < 0 || id >= kNumThreads) {
    printf("[%d] ERROR: bad id\n", id);
    return 0;
  }

  for (r = 0; r < kNumRounds; ++r) {
    printf("[%d] before round %d\n", id, r);
    CounterWait(id, r);
    CheckAndUpdateTlsVars(id, r);
    CounterAdvance(id, r);
    printf("[%d] after round %d\n", id, r);
  }

  printf("[%d] exiting thread\n", id);
  return 0;
}


int main(int argc, char *argv[]) {
  pthread_t tid[kNumThreads];
  int ids[kNumThreads];
  int i = 0;

  Init();

  for (i = 0; i < kNumThreads; ++i) {
    int rv;
    ids[i] = i;
    printf("creating thread %d\n", i);
    rv = pthread_create(&tid[i], NULL, MyThreadFunction, &ids[i]);
    if (rv != 0) {
      printf("ERROR: in thread creation\n");
      IncError();
    }
  }

  for (i = 0; i < kNumThreads; ++i) {
    pthread_join(tid[i], NULL);
  }

  return GlobalError;
}
