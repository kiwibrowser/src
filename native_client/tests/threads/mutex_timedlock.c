/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "native_client/src/include/nacl_assert.h"


static pthread_mutex_t mutex;
volatile int thread_has_lock = 0;
volatile int thread_should_acquire_lock = 0;
volatile int thread_should_release_lock = 0;

void *locking_thread(void *unused) {
  for (;;) {
    while (!thread_should_acquire_lock) { /* Spin. */ }

    ASSERT_EQ(thread_has_lock, 0);
    int rc = pthread_mutex_lock(&mutex);
    ASSERT_EQ(rc, 0);
    __sync_fetch_and_add(&thread_has_lock, 1);

    while (!thread_should_release_lock) { /* Spin. */ }

    ASSERT_EQ(thread_has_lock, 1);
    rc = pthread_mutex_unlock(&mutex);
    ASSERT_EQ(rc, 0);
    __sync_fetch_and_sub(&thread_has_lock, 1);
  }

  return NULL;
}

void tell_thread_to_acquire_lock(void) {
  fprintf(stderr, "Thread acquiring lock.\n");

  ASSERT_EQ(thread_has_lock, 0);
  ASSERT_EQ(thread_should_acquire_lock, 0);
  __sync_fetch_and_add(&thread_should_acquire_lock, 1);

  while (!thread_has_lock) { /* Spin. */ }

  __sync_fetch_and_sub(&thread_should_acquire_lock, 1);
  ASSERT_EQ(thread_should_acquire_lock, 0);

  fprintf(stderr, "Thread acquired lock.\n");
}

void tell_thread_to_release_lock(void) {
  fprintf(stderr, "Thread releasing lock.\n");

  ASSERT_EQ(thread_has_lock, 1);
  ASSERT_EQ(thread_should_release_lock, 0);
  __sync_fetch_and_add(&thread_should_release_lock, 1);

  while (thread_has_lock) { /* Spin. */ }

  __sync_fetch_and_sub(&thread_should_release_lock, 1);
  ASSERT_EQ(thread_should_release_lock, 0);

  fprintf(stderr, "Thread released lock.\n");
}

void add_nanoseconds(struct timespec *time, unsigned int nanoseconds) {
  ASSERT_LE(nanoseconds, 1000000000);
  time->tv_nsec += nanoseconds;
  if (time->tv_nsec > 1000000000) {
    time->tv_nsec -= 1000000000;
    time->tv_sec += 1;
  }
}

void test_already_locked_with_zero_timestamp(void) {
  int rc;
  struct timespec abstime = { 0, 0 };
  tell_thread_to_acquire_lock();
  fprintf(stderr, "Trying to lock the already-locked mutex for a valid "
          "zero absolute timestamp. "
          "Expected to expire since the lock is taken.\n");
  rc = pthread_mutex_timedlock(&mutex, &abstime);
  ASSERT_EQ(rc, ETIMEDOUT);
  tell_thread_to_release_lock();
}

void test_already_locked_with_non_zero_timestamp(void) {
  int rc;
  struct timespec abstime = { 0, 0 };
  tell_thread_to_acquire_lock();
  fprintf(stderr, "Trying to lock the already-locked mutex for a valid "
          "non-zero absolute timestamp. "
          "Expected to expire since the lock is taken.\n");
  rc = clock_gettime(CLOCK_REALTIME, &abstime);
  ASSERT_EQ(rc, 0);
  add_nanoseconds(&abstime, 10000);
  rc = pthread_mutex_timedlock(&mutex, &abstime);
  ASSERT_EQ(rc, ETIMEDOUT);
  tell_thread_to_release_lock();
}

void test_already_locked_with_negative_timestamp(void) {
  int rc;
  struct timespec abstime = { 0, -10000 };
  tell_thread_to_acquire_lock();
  fprintf(stderr, "Trying to lock the already-locked mutex for an invalid "
          "negative absolute timestamp. "
          "Expected to be invalid.\n");
  rc = pthread_mutex_timedlock(&mutex, &abstime);
  ASSERT_EQ(rc, EINVAL);
  tell_thread_to_release_lock();
}

void test_already_locked_with_too_large_timestamp(void) {
  int rc;
  struct timespec abstime = { 0, 2000000000 };
  tell_thread_to_acquire_lock();
  fprintf(stderr, "Trying to lock the already-locked mutex for an invalid "
          "too-large absolute timestamp. "
          "Expected to be invalid.\n");
  rc = pthread_mutex_timedlock(&mutex, &abstime);
  ASSERT_EQ(rc, EINVAL);
  tell_thread_to_release_lock();
}

void test_unlocked_with_zero_timestamp(void) {
  int rc;
  struct timespec abstime = { 0, 0 };
  ASSERT_EQ(thread_has_lock, 0);
  fprintf(stderr, "Trying to lock the unlocked mutex with a valid "
          "zero absolute timestamp. "
          "Expected to succeed instantly since the lock is free.\n");
  rc = pthread_mutex_timedlock(&mutex, &abstime);
  ASSERT_EQ(rc, 0);
  rc = pthread_mutex_unlock(&mutex);
  ASSERT_EQ(rc, 0);
}

void test_unlocked_with_non_zero_timestamp(void) {
  int rc;
  struct timespec abstime = { 0, 0 };
  ASSERT_EQ(thread_has_lock, 0);
  fprintf(stderr, "Trying to lock the unlocked mutex with a valid "
          "non-zero absolute timestamp. "
          "Expected to succeed instantly since the lock is free.\n");
  rc = clock_gettime(CLOCK_REALTIME, &abstime);
  ASSERT_EQ(rc, 0);
  add_nanoseconds(&abstime, 10000);
  rc = pthread_mutex_timedlock(&mutex, &abstime);
  ASSERT_EQ(rc, 0);
  rc = pthread_mutex_unlock(&mutex);
  ASSERT_EQ(rc, 0);
}

int main(int argc, char **argv) {
  int rc;
  fprintf(stderr, "Running...\n");

  pthread_mutexattr_t mta;
  rc = pthread_mutexattr_init(&mta);
  ASSERT_EQ(rc, 0);
  rc = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_ERRORCHECK);
  ASSERT_EQ(rc, 0);
  rc = pthread_mutex_init(&mutex, &mta);
  ASSERT_EQ(rc, 0);
  rc = pthread_mutexattr_destroy(&mta);
  ASSERT_EQ(rc, 0);

  pthread_t thread;
  rc = pthread_create(&thread, NULL, locking_thread, NULL);
  ASSERT_EQ(rc, 0);
  fprintf(stderr, "Thread started.\n");

  test_already_locked_with_zero_timestamp();
  test_already_locked_with_non_zero_timestamp();
  test_already_locked_with_negative_timestamp();
  test_already_locked_with_too_large_timestamp();
  test_unlocked_with_zero_timestamp();
  test_unlocked_with_non_zero_timestamp();

  fprintf(stderr, "Done.\n");
  return 0;
}
