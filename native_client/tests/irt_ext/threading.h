/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This module handles all the testing for thread related functions, namely
 * thread creation, thread destruction, and thread priorities, and also futex
 * wait and futex wake functions.
 *
 * For the purposes of this test harness, much like other time related test
 * harnesses time passes by instantly. This means that on futex wait calls with
 * supplied timeout values, times on the threading_environment will instantly
 * pass by and will always return ETIMEDOUT.
 */

#ifndef NATIVE_CLIENT_TESTS_IRT_EXT_THREADING_H
#define NATIVE_CLIENT_TESTS_IRT_EXT_THREADING_H

struct threading_environment {
  volatile int num_threads_created;
  volatile int num_threads_exited;
  volatile int num_futex_wait_calls;
  volatile int num_futex_wake_calls;

  volatile int last_set_thread_nice;
  volatile int current_time;
};

void init_threading_module(void);

void init_threading_environment(struct threading_environment *env);

void activate_threading_env(struct threading_environment *env);
void deactivate_threading_env(void);

#endif /* NATIVE_CLIENT_TESTS_IRT_EXT_THREADING_H */
