/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "native_client/src/include/nacl_assert.h"

pthread_rwlock_t g_rwlock = PTHREAD_RWLOCK_INITIALIZER;
volatile int g_thread_has_lock = 0;
volatile int g_thread_should_acquire_lock = 0;
volatile int g_thread_should_release_lock = 0;

typedef enum {
  READ_LOCK = 1,
  WRITE_LOCK = 2,
} lock_type;

void *locking_thread(void *unused) {
  int rc;
  for (;;) {
    while (!g_thread_should_acquire_lock) { /* Spin. */ }

    ASSERT_EQ(g_thread_has_lock, 0);
    if (g_thread_should_acquire_lock == WRITE_LOCK)
      rc = pthread_rwlock_wrlock(&g_rwlock);
    else
      rc = pthread_rwlock_rdlock(&g_rwlock);
    ASSERT_EQ(rc, 0);
    __sync_fetch_and_add(&g_thread_has_lock, 1);

    while (!g_thread_should_release_lock) { /* Spin. */ }

    ASSERT_EQ(g_thread_has_lock, 1);
    rc = pthread_rwlock_unlock(&g_rwlock);
    ASSERT_EQ(rc, 0);
    __sync_fetch_and_sub(&g_thread_has_lock, 1);
  }

  return NULL;
}


void tell_thread_to_acquire_lock(lock_type lock_type) {
  fprintf(stderr, "Thread acquiring lock: %s\n",
      lock_type == WRITE_LOCK ? "WRITE" : "READ");

  ASSERT_EQ(g_thread_has_lock, 0);
  ASSERT_EQ(g_thread_should_acquire_lock, 0);
  __sync_fetch_and_add(&g_thread_should_acquire_lock, lock_type);

  while (!g_thread_has_lock) { /* Spin. */ }

  __sync_fetch_and_sub(&g_thread_should_acquire_lock, lock_type);
  ASSERT_EQ(g_thread_should_acquire_lock, 0);

  fprintf(stderr, "Thread acquired lock.\n");
}

void tell_thread_to_release_lock(void) {
  fprintf(stderr, "Thread releasing lock.\n");

  ASSERT_EQ(g_thread_has_lock, 1);
  ASSERT_EQ(g_thread_should_release_lock, 0);
  __sync_fetch_and_add(&g_thread_should_release_lock, 1);

  while (g_thread_has_lock) { /* Spin. */ }

  __sync_fetch_and_sub(&g_thread_should_release_lock, 1);
  ASSERT_EQ(g_thread_should_release_lock, 0);

  fprintf(stderr, "Thread released lock.\n");
}

void test_reader_timedwait(void) {
  fprintf(stderr, "test_reader_timedwait\n");
  tell_thread_to_acquire_lock(WRITE_LOCK);

  struct timespec t = { 0, 0 };
  int rc = pthread_rwlock_timedrdlock(&g_rwlock, &t);
  ASSERT_EQ(rc, ETIMEDOUT);

  tell_thread_to_release_lock();
}

void test_writer_timedwait(void) {
  fprintf(stderr, "test_writer_timedwait\n");
  tell_thread_to_acquire_lock(READ_LOCK);

  struct timespec t = { 0, 0 };
  int rc = pthread_rwlock_timedwrlock(&g_rwlock, &t);
  ASSERT_EQ(rc, ETIMEDOUT);

  tell_thread_to_release_lock();
}

void test_multiple_writers(void) {
  fprintf(stderr, "test_multiple_writers\n");
  tell_thread_to_acquire_lock(WRITE_LOCK);

  /*
   * Attempt to acquire second write lock should fail.
   */
  int rc = pthread_rwlock_trywrlock(&g_rwlock);
  ASSERT_EQ(rc, EBUSY);

  tell_thread_to_release_lock();
}

void test_recursive_reader(void) {
  /*
   * Test that an rdlock can be recursively acquired even when there
   * is a waiting writer.
   */
  int rc = pthread_rwlock_rdlock(&g_rwlock);
  ASSERT_EQ(rc, 0);

  /*
   * Tell the locking thread to attempt to acquire the write lock.
   * This should fail and block until all readers are unlocked.
   */
  ASSERT_EQ(g_thread_has_lock, 0);
  ASSERT_EQ(g_thread_should_acquire_lock, 0);
  __sync_fetch_and_add(&g_thread_should_acquire_lock, WRITE_LOCK);

  /*
   * Sleep for 10ms
   */
  rc = usleep(10 * 1000);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(g_thread_has_lock, 0);

  /*
   * Now make sure the waiting writer doesn't block the recursive acquisition
   * of the rdlock (using both tryrdlock and rdlock).
   */
  rc = pthread_rwlock_tryrdlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(g_thread_has_lock, 0);
  rc = pthread_rwlock_rdlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(g_thread_has_lock, 0);

  /*
   * Finally unlock the rdlock which should allow the secondary thread
   * to acquire the wrlock
   */
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  while (!g_thread_has_lock) { /* Spin. */ }
  __sync_fetch_and_sub(&g_thread_should_acquire_lock, WRITE_LOCK);
  ASSERT_EQ(g_thread_should_acquire_lock, 0);

  tell_thread_to_release_lock();
}

void test_multiple_readers(void) {
  fprintf(stderr, "test_multiple_readers\n");
  tell_thread_to_acquire_lock(READ_LOCK);

  /*
   * Now attempt to acquire the lock on the main thread.
   * Since they are both readers this should succeed.
   * Try with tryrdlock, rdlock and timedrdlock.
   */
  int rc = pthread_rwlock_tryrdlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);

  struct timespec t = { 0, 0 };
  rc = pthread_rwlock_timedrdlock(&g_rwlock, &t);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);

  rc = pthread_rwlock_rdlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);

  tell_thread_to_release_lock();
}

void *read_lock_thread(void *unused) {
  fprintf(stderr, "%lx: waiting for read lock\n", (long)pthread_self());
  int rc = pthread_rwlock_rdlock(&g_rwlock);
  fprintf(stderr, "%lx: read lock acquired\n", (long)pthread_self());
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
  fprintf(stderr, "%lx: thread done\n", (long)pthread_self());
  return NULL;
}

/*
 * Number of milliseconds delay before we assume that newly created threads
 * (running read_lock_thread) are blocked on lock acquisition.
 */
#define THREAD_START_DELAY_MS 20

/*
 * Test that multiple readers are woken up when the write lock
 * is released.
 */
void test_multiple_reader_wakeup(void) {
  /*
   * First, acquire the write lock so that all readers
   * will be blocked.
   */
  fprintf(stderr, "test_multiple_reader_wakeup\n");
  int rc;
  rc = pthread_rwlock_wrlock(&g_rwlock);
  ASSERT_EQ(rc, 0);

  /*
   * Now start two new threads that will both wait on a read lock.
   */
  pthread_t thread1;
  rc = pthread_create(&thread1, NULL, read_lock_thread, NULL);
  ASSERT_EQ(rc, 0);

  pthread_t thread2;
  rc = pthread_create(&thread2, NULL, read_lock_thread, NULL);
  ASSERT_EQ(rc, 0);

  /*
   * We now need to wait until both threads are waiting on the read lock.
   * However, we have no way to determine this as we do not record the number
   * of waiting readers.  Sadly, we must resort to an arbitrary delay in the
   * test code, and hope that after this delay the two threads are both blocked.
   */
  rc = usleep(THREAD_START_DELAY_MS * 1000);
  ASSERT_EQ(rc, 0);

  /*
   * Releasing the write lock should unblock both the readers and they
   * should then both be joinable.
   */
  rc = pthread_rwlock_unlock(&g_rwlock);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
}

void test_reader_plus_writer(void) {
  fprintf(stderr, "test_reader_plus_writer\n");
  tell_thread_to_acquire_lock(READ_LOCK);

  /*
   * Now attempt to acquire the write lock on the main thread.
   * This should fail.
   */
  int rc = pthread_rwlock_trywrlock(&g_rwlock);
  ASSERT_EQ(rc, EBUSY);

  tell_thread_to_release_lock();
}

void test_writer_plus_reader(void) {
  fprintf(stderr, "test_writer_plus_reader\n");

  /*
   * First get the write lock.
   */
  int rc = pthread_rwlock_wrlock(&g_rwlock);
  ASSERT_EQ(rc, 0);

  /*
   * Attempt to acquire read lock should now fail
   */
  rc = pthread_rwlock_tryrdlock(&g_rwlock);
  ASSERT_EQ(rc, EBUSY);

  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
}

void test_unlocked_with_zero_timestamp(void) {
  fprintf(stderr, "test_unlocked_with_zero_timestamp\n");
  int rc;
  struct timespec abstime = { 0, 0 };
  ASSERT_EQ(g_thread_has_lock, 0);
  fprintf(stderr, "Trying to lock the unlocked rwlock with a valid "
          "zero absolute timestamp. "
          "Expected to succeed instantly since the lock is free.\n");
  rc = pthread_rwlock_timedrdlock(&g_rwlock, &abstime);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_unlock(&g_rwlock);
  ASSERT_EQ(rc, 0);
}

void test_rwlock_init(void) {
  int rc;
  pthread_rwlockattr_t attrs;
  rc = pthread_rwlockattr_init(&attrs);
  ASSERT_EQ(rc, 0);
  int shared = -1;
  rc = pthread_rwlockattr_getpshared(&attrs, &shared);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(shared, PTHREAD_PROCESS_PRIVATE);
  rc = pthread_rwlockattr_setpshared(&attrs, PTHREAD_PROCESS_SHARED);
#ifndef __GLIBC__
  /*
   * glibc's pthread_rwlockattr_setpshared doesn't currently fail
   * with PTHREAD_PROCESS_SHARED. TODO(sbc): remove this once
   * we fix the glibc bug:
   * https://code.google.com/p/nativeclient/issues/detail?id=4142
   */
  ASSERT_EQ(rc, EINVAL);
#endif
  rc = pthread_rwlockattr_setpshared(&attrs, PTHREAD_PROCESS_PRIVATE);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlock_init(&g_rwlock, &attrs);
  ASSERT_EQ(rc, 0);
  rc = pthread_rwlockattr_destroy(&attrs);
  ASSERT_EQ(rc, 0);
}

void run_tests(void) {
  test_unlocked_with_zero_timestamp();
  test_multiple_readers();
  test_multiple_writers();
  test_reader_plus_writer();
  test_writer_plus_reader();
  test_reader_timedwait();
  test_writer_timedwait();
  test_recursive_reader();
  test_multiple_reader_wakeup();
}

int main(int argc, char **argv) {
  int rc;
  pthread_t thread;
  fprintf(stderr, "Running...\n");

  rc = pthread_create(&thread, NULL, locking_thread, NULL);
  ASSERT_EQ(rc, 0);
  fprintf(stderr, "Thread started.\n");

  /*
   * Call run_tests twice, first with the lock initialised via
   * PTHREAD_RWLOCK_INITIALIZER and then via pthread_rwlockattr_init.
   * The result should match since we don't set any non-default attributes
   * when calling pthread_rwlockattr_init.
   */
  run_tests();
  test_rwlock_init();
  run_tests();
  rc = pthread_rwlock_destroy(&g_rwlock);
  ASSERT_EQ(rc, 0);

  fprintf(stderr, "Done.\n");
}
