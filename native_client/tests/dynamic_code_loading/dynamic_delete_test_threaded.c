/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/dynamic_code_loading/dynamic_load_test.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include <nacl/nacl_dyncode.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int stage = 0;

void *check_in_thread(void *load_area) {
  pthread_mutex_lock(&mutex);
  stage = 1;
  pthread_cond_signal(&cond);
  while (stage != 2) {
    pthread_cond_wait(&cond, &mutex);
  }
  /* Check in this thread. */
  assert(nacl_dyncode_delete(NULL, 0) == 0);
  stage = 3;
  pthread_cond_signal(&cond);

  while (stage != 4) {
    pthread_cond_wait(&cond, &mutex);
  }
  /* Finish deletion that have been started on the primary thread. */
  assert(nacl_dyncode_delete(load_area, NACL_BUNDLE_SIZE) == 0);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

/*
 * Check that we can't delete dynamic code without checking in from other
 * threads.
 */
void test_threaded_delete(void) {
  pthread_t other_thread;
  void *load_area = allocate_code_space(1);
  uint8_t buf[NACL_BUNDLE_SIZE];
  int rc;
  fill_nops(buf, sizeof(buf));
  assert(pthread_create(&other_thread, NULL, check_in_thread, load_area) == 0);
  assert(nacl_dyncode_create(load_area, buf, sizeof(buf)) == 0);
  pthread_mutex_lock(&mutex);
  while (stage != 1) {
    pthread_cond_wait(&cond, &mutex);
  }
  /* Try to delete without check in from the other thread. */
  rc = nacl_dyncode_delete(load_area, sizeof(buf));
  assert(rc == -1);
  assert(errno == EAGAIN);

  stage = 2;
  pthread_cond_signal(&cond);
  while (stage != 3) {
    pthread_cond_wait(&cond, &mutex);
  }
  /* Delete with check in from the other thread. */
  assert(nacl_dyncode_delete(load_area, sizeof(buf)) == 0);

  /* Test that we can finish deleting on other thread. */
  assert(nacl_dyncode_create(load_area, buf, sizeof(buf)) == 0);
  rc = nacl_dyncode_delete(load_area, sizeof(buf));
  assert (rc == -1);
  assert (errno == EAGAIN);
  stage = 4;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  assert(pthread_join(other_thread, NULL) == 0);
}
