/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/tests/irt_ext/error_report.h"
#include "native_client/tests/irt_ext/basic_calls.h"
#include "native_client/tests/irt_ext/libc/libc_test.h"

typedef int (*TYPE_basic_test)(struct basic_calls_environment *basic_call_env);

#define TEST_TIME_VALUE 20
#define TEST_NANOTIME_VALUE 123

static int do_time_test(struct basic_calls_environment *basic_call_env) {
  time_t retrieved_time;

  basic_call_env->current_time = TEST_TIME_VALUE;
  retrieved_time = time(NULL);

  if (retrieved_time != TEST_TIME_VALUE) {
    irt_ext_test_print("do_time_test: did not retrieve expected time.\n"
                       "  Retrieved time: %d. Expected time: %d.\n",
                       (int) retrieved_time, TEST_TIME_VALUE);
    return 1;
  }

  if (basic_call_env->current_time <= retrieved_time) {
    irt_ext_test_print("do_time_test: current time less than sampled time.\n"
                       "  Current time: %d. Sampled time: %d.\n",
                       (int) basic_call_env->current_time,
                       (int) retrieved_time);
    return 1;
  }

  return 0;
}

static int do_clock_test(struct basic_calls_environment *basic_call_env) {
  clock_t retrieved_clock;

  basic_call_env->current_time = TEST_TIME_VALUE;
  retrieved_clock = clock();

  if (retrieved_clock != TEST_TIME_VALUE) {
    irt_ext_test_print("do_clock_test: did not retrieve expected clock.\n"
                       "  Retrieved clock: %d. Expected clock: %d.\n",
                       (int) retrieved_clock, TEST_TIME_VALUE);
    return 1;
  }

  if (basic_call_env->current_time <= retrieved_clock) {
    irt_ext_test_print("do_clock_test: current time less than sampled clock.\n"
                       "  Current clock: %d. Sampled clock: %d.\n",
                       (int) basic_call_env->current_time,
                       (int) retrieved_clock);
    return 1;
  }

  return 0;
}

static int do_nanosleep_test(struct basic_calls_environment *basic_call_env) {
  struct timespec sleep_time = {
    TEST_TIME_VALUE,
    TEST_NANOTIME_VALUE
  };

  struct timespec remaining_time = {
    0,
    0
  };

  basic_call_env->current_time = TEST_TIME_VALUE;
  if (0 != nanosleep(&sleep_time, &remaining_time)) {
    irt_ext_test_print("do_nanosleep_test: nanosleep failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (remaining_time.tv_sec != 0 ||
      remaining_time.tv_nsec != sleep_time.tv_nsec) {
    irt_ext_test_print("do_nanosleep_test: unexpected sleep remainder.\n"
                       "  Expected: 0:%d. Retrieved: %d:%d.\n",
                       (int) sleep_time.tv_nsec,
                       (int) remaining_time.tv_sec,
                       (int) remaining_time.tv_nsec);
    return 1;
  }

  if (basic_call_env->current_time != TEST_TIME_VALUE + sleep_time.tv_sec) {
    irt_ext_test_print("do_nanosleep_test: env did not sleep correctly.\n"
                       "  Expected: %d. Retrieved: %d.\n",
                       (int) (TEST_TIME_VALUE + sleep_time.tv_sec),
                       (int) basic_call_env->current_time);
    return 1;
  }

  return 0;
}

static int do_sched_yield_test(struct basic_calls_environment *basic_call_env) {
  if (basic_call_env->thread_yielded) {
    irt_ext_test_print("do_sched_yield_test: env was not initialized.\n");
    return 1;
  }

  if (0 != sched_yield()) {
    irt_ext_test_print("do_sched_yield_test: sched_yield failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (!basic_call_env->thread_yielded) {
    irt_ext_test_print("do_sched_yield_test: env did not yield thread.\n");
    return 1;
  }

  return 0;
}

static int do_sysconf_test(struct basic_calls_environment *basic_call_env) {
  long ret = sysconf(SYSCONF_TEST_LIBC_QUERY);
  if (ret == -1) {
    irt_ext_test_print("do_sysconf_test: sysconf failed - %s.\n",
                       strerror(errno));
    return 1;
  }

  if (SYSCONF_TEST_VALUE != ret) {
    irt_ext_test_print("do_sysconf_test: sysconf returned unexpected value.\n"
                       "  Expected value: %d. Retrieved value: %d.\n",
                       SYSCONF_TEST_VALUE, (int) ret);
    return 1;
  }

  return 0;
}

static int do_pid_test(struct basic_calls_environment *basic_call_env) {
  if (basic_call_env->pid != 0) {
    irt_ext_test_print("do_pid_test: env was not initialized.\n");
    return 1;
  }

  pid_t old_pid = getpid();
  if (old_pid != 0) {
    irt_ext_test_print("do_pid_test: getpid returned unexpected result.\n"
                       "  Expected value: 0. Retrieved value: %d.\n",
                       (int) old_pid);
    return 1;
  }

  basic_call_env->pid = 1;
  pid_t new_pid = getpid();
  if (new_pid != basic_call_env->pid) {
    irt_ext_test_print("do_pid_test: getpid returned unexpected result.\n"
                       "  Expected value: %d. Retrieved value: %d.\n",
                       (int) basic_call_env->pid, (int) new_pid);
    return 1;
  }

  return 0;
}

static int do_getres_test(struct basic_calls_environment *basic_call_env) {
  struct timespec res;
  if (0 == clock_getres(CLOCK_REALTIME, &res)) {
    irt_ext_test_print("do_getres_test: clock_getres returned system clock with"
                       " activated environment.\n");
    return 1;
  }

  if (0 != clock_getres(ENV_CLOCK_ID, &res)) {
    irt_ext_test_print("do_getres_test: clock_getres failed with environment"
                       " clock: %s.\n",
                       strerror(errno));
    return 1;
  }

  if (res.tv_sec != 1 || res.tv_nsec != 0) {
    irt_ext_test_print("do_getres_test: unexpected resolution.\n"
                       "  Expected: 1:0. Retrieved: %d:%d.\n",
                       (int) res.tv_sec, (int) res.tv_nsec);
    return 1;
  }

  return 0;
}

static int do_gettime_test(struct basic_calls_environment *basic_call_env) {
  struct timespec env_time;

  basic_call_env->current_time = TEST_TIME_VALUE;
  if (0 != clock_gettime(ENV_CLOCK_ID, &env_time)) {
    irt_ext_test_print("do_gettime_test: clock_gettime failed with environment"
                       " clock: %s.\n",
                       strerror(errno));
    return 1;
  }

  if (env_time.tv_sec != TEST_TIME_VALUE || env_time.tv_nsec != 0) {
    irt_ext_test_print("do_gettime_test: unexpected time value.\n"
                       "  Expected: %d:0. Retrieved: %d:%d.\n",
                       TEST_TIME_VALUE,
                       (int) env_time.tv_sec, (int) env_time.tv_nsec);
    return 1;
  }

  if (basic_call_env->current_time <= env_time.tv_sec) {
    irt_ext_test_print("do_gettime_test: current time less than sampled time.\n"
                       "  Current time: %d. Sampled time: %d.\n",
                       (int) basic_call_env->current_time,
                       (int) env_time.tv_sec);
    return 1;
  }

  return 0;
}

static const TYPE_basic_test g_basic_tests[] = {
  /* Tests for nacl_irt_basic. */
  do_time_test,
  do_clock_test,
  do_nanosleep_test,
  do_sched_yield_test,
  do_sysconf_test,

  /* Tests for nacl_irt_dev_getpid. */
  do_pid_test,

  /* Tests for nacl_irt_clock. */
  do_getres_test,
  do_gettime_test,
};

static void setup(struct basic_calls_environment *basic_call_env) {
  init_basic_calls_environment(basic_call_env);
  activate_basic_calls_env(basic_call_env);
}

static void teardown(void) {
  deactivate_basic_calls_env();
}

DEFINE_TEST(Basic, g_basic_tests, struct basic_calls_environment,
            setup, teardown)
