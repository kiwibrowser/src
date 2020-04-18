/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/dynamic_code_loading/dynamic_load_test.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#define BIG_CODE_SIZE 40960
#define NUM_ITERATIONS 8
#define NUM_THREADS 8

struct ThreadInfo {
  int thread_index;
  int illegal;
  char *load_addr[NUM_ITERATIONS];
};

void init_buffer(struct ThreadInfo *info, uint8_t *buf, size_t size) {
  fill_nops(buf, size);
#if defined(__i386__) || defined(__x86_64__)
  /* Make each buffer unique with a "mov $number, %eax" */
  for (int i = 0; i < size; i += NACL_BUNDLE_SIZE) {
    buf[i + 0] = 0xb8;
    buf[i + 1] = (uint8_t) info->thread_index;
    buf[i + 2] = 0x00;
    buf[i + 3] = (uint8_t) i / NACL_BUNDLE_SIZE;
    buf[i + 4] = 0x00;
  }
  if (info->illegal) {
    /* ret is illegal. */
    buf[size - 1] = 0xc3;
  }
#elif defined(__arm__)
  /* Make each buffer unique with a "mov r0, #number" */
  for (int i = 0; i < size; i += NACL_BUNDLE_SIZE) {
    buf[i + 0] = (uint8_t) info->thread_index;
    buf[i + 1] = 0x00;
    buf[i + 2] = 0xa0;
    buf[i + 3] = 0xe3;
  }
  if (info->illegal) {
    /* software interupts (swi 0x420000) are illegal */
    buf[size - 4] = 0x00;
    buf[size - 3] = 0x00;
    buf[size - 2] = 0x42;
    buf[size - 1] = 0xef;
  }
#else
#error "Unknown Platform"
#endif
}

void *run_repeated_load(void *thread_arg) {
  struct ThreadInfo *info = thread_arg;
  /* Shake things up, every other thread will invalidate illegal code. */
  uint8_t *buf = malloc(BIG_CODE_SIZE);
  assert(buf != NULL);
  init_buffer(info, buf, BIG_CODE_SIZE);
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    int rc = nacl_load_code(info->load_addr[i], buf, BIG_CODE_SIZE);
    if (info->illegal) {
      assert(rc == -EINVAL);
    } else {
      assert(rc == 0);
      /* Was the code actually loaded? */
      assert(memcmp(info->load_addr[i], buf, BIG_CODE_SIZE) == 0);
    }
  }
  free(buf);
  return NULL;
}

/*
 * A sanity test that dynamic code loading and validation is thread safe.
 * This test can't detect every possible problem, but it will hopefully detect
 * obvious ones.
 */
void test_threaded_loads(void) {
  pthread_t thread[NUM_THREADS];
  struct ThreadInfo info[NUM_THREADS + 1];
  int rc;

  /* Init the data structure for each thread. */
  for (int t = 0; t < NUM_THREADS + 1; t++) {
    info[t].thread_index = t;
    info[t].illegal = t % 2;
    /* Preallocate address space: easier than ensuring thread safety. */
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      info[t].load_addr[i] = allocate_code_space(BIG_CODE_SIZE / PAGE_SIZE + 1);
    }
  }

  /* Launch a bunch of threads. */
  for (int t = 0; t < NUM_THREADS; t++) {
    rc = pthread_create(&thread[t], NULL, run_repeated_load, &info[t]);
    assert(rc == 0);
  }

  /* Run on the main thread, too */
  info[NUM_THREADS].illegal = 0;
  run_repeated_load(&info[NUM_THREADS]);

  /* Join the threads to make sure we're done. */
  for (int t = 0; t < NUM_THREADS; t++) {
    rc = pthread_join(thread[t], NULL);
    assert(rc == 0);
  }
}
