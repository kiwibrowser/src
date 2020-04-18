/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/tests/performance/perf_test_runner.h"


// Measure the speed of an (uncontended) atomic operation so that we
// can compare this with TestUncontendedMutexLock.
class TestAtomicIncrement : public PerfTest {
 public:
  TestAtomicIncrement() {
    // We don't particularly need to initialize var_, but it might
    // stop memory checkers from complaining.
    var_ = 0;
  }

  virtual void run() {
    __sync_fetch_and_add(&var_, 1);
  }

 private:
  int var_;
};
PERF_TEST_DECLARE(TestAtomicIncrement)

class TestUncontendedMutexLock : public PerfTest {
 public:
  TestUncontendedMutexLock() {
    ASSERT_EQ(pthread_mutex_init(&mutex_, NULL), 0);
  }

  ~TestUncontendedMutexLock() {
    ASSERT_EQ(pthread_mutex_destroy(&mutex_), 0);
  }

  virtual void run() {
    ASSERT_EQ(pthread_mutex_lock(&mutex_), 0);
    ASSERT_EQ(pthread_mutex_unlock(&mutex_), 0);
  }

 private:
  pthread_mutex_t mutex_;
};
PERF_TEST_DECLARE(TestUncontendedMutexLock)

// Test the overhead of pthread_cond_signal() on a condvar that no
// thread is waiting on.
class TestCondvarSignalNoOp : public PerfTest {
 public:
  TestCondvarSignalNoOp() {
    ASSERT_EQ(pthread_cond_init(&condvar_, NULL), 0);
  }

  ~TestCondvarSignalNoOp() {
    ASSERT_EQ(pthread_cond_destroy(&condvar_), 0);
  }

  virtual void run() {
    ASSERT_EQ(pthread_cond_signal(&condvar_), 0);
  }

 private:
  pthread_cond_t condvar_;
};
PERF_TEST_DECLARE(TestCondvarSignalNoOp)

class TestThreadCreateAndJoin : public PerfTest {
 public:
  virtual void run() {
    pthread_t tid;
    ASSERT_EQ(pthread_create(&tid, NULL, EmptyThread, NULL), 0);
    ASSERT_EQ(pthread_join(tid, NULL), 0);
  }

 private:
  static void *EmptyThread(void *thread_arg) {
    UNREFERENCED_PARAMETER(thread_arg);
    return NULL;
  }
};
PERF_TEST_DECLARE(TestThreadCreateAndJoin)

class TestThreadWakeup : public PerfTest {
 public:
  TestThreadWakeup() {
    ASSERT_EQ(pthread_mutex_init(&mutex_, NULL), 0);
    ASSERT_EQ(pthread_cond_init(&condvar1_, NULL), 0);
    ASSERT_EQ(pthread_cond_init(&condvar2_, NULL), 0);
    state_ = WAIT;
    ASSERT_EQ(pthread_create(&tid_, NULL, Thread, this), 0);
  }

  ~TestThreadWakeup() {
    ASSERT_EQ(pthread_mutex_lock(&mutex_), 0);
    state_ = EXIT;
    ASSERT_EQ(pthread_cond_signal(&condvar1_), 0);
    ASSERT_EQ(pthread_mutex_unlock(&mutex_), 0);

    ASSERT_EQ(pthread_join(tid_, NULL), 0);
    ASSERT_EQ(pthread_cond_destroy(&condvar2_), 0);
    ASSERT_EQ(pthread_cond_destroy(&condvar1_), 0);
    ASSERT_EQ(pthread_mutex_destroy(&mutex_), 0);
  }

  virtual void run() {
    ASSERT_EQ(pthread_mutex_lock(&mutex_), 0);
    state_ = WAKE_CHILD;
    ASSERT_EQ(pthread_cond_signal(&condvar1_), 0);
    while (state_ != REPLY_TO_PARENT)
      ASSERT_EQ(pthread_cond_wait(&condvar2_, &mutex_), 0);
    state_ = WAIT;
    ASSERT_EQ(pthread_mutex_unlock(&mutex_), 0);
  }

 private:
  static void *Thread(void *thread_arg) {
    TestThreadWakeup *obj = (TestThreadWakeup *) thread_arg;
    bool do_exit = false;
    while (!do_exit) {
      ASSERT_EQ(pthread_mutex_lock(&obj->mutex_), 0);
      for (;;) {
        if (obj->state_ == EXIT) {
          do_exit = true;
          break;
        } else if (obj->state_ == WAKE_CHILD) {
          obj->state_ = REPLY_TO_PARENT;
          ASSERT_EQ(pthread_cond_signal(&obj->condvar2_), 0);
          break;
        }
        ASSERT_EQ(pthread_cond_wait(&obj->condvar1_, &obj->mutex_), 0);
      }
      ASSERT_EQ(pthread_mutex_unlock(&obj->mutex_), 0);
    }
    return NULL;
  }

  pthread_t tid_;
  pthread_mutex_t mutex_;
  pthread_cond_t condvar1_;
  pthread_cond_t condvar2_;
  enum { WAIT, WAKE_CHILD, REPLY_TO_PARENT, EXIT } state_;
};
PERF_TEST_DECLARE(TestThreadWakeup)
