/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_extension.h"
#include "native_client/tests/irt_ext/basic_calls.h"
#include "native_client/tests/irt_ext/error_report.h"

static struct nacl_irt_basic g_irt_basic;
static struct basic_calls_environment *g_activated_env = NULL;
static const struct basic_calls_environment g_default_env = {
  .exit_called = false,
};

static void my_exit(int status) {
  if (status == TEST_EXIT_CODE)
    status = 0;

  g_irt_basic.exit(status);
}

static int my_gettod(struct timeval *tv) {
  if (g_activated_env) {
    tv->tv_sec = g_activated_env->current_time;
    tv->tv_usec = 0;

    g_activated_env->current_time++;
    return 0;
  }
  return ENOSYS;
}

/* Returns number of ticks which will just be seconds in current time. */
static int my_clock(nacl_irt_clock_t *ticks) {
  if (g_activated_env) {
    *ticks = g_activated_env->current_time;

    g_activated_env->current_time++;
    return 0;
  }
  return ENOSYS;
}

static int my_nanosleep(const struct timespec *req, struct timespec *rem) {
  if (g_activated_env) {
    g_activated_env->current_time += req->tv_sec;

    rem->tv_sec = 0;
    rem->tv_nsec = req->tv_nsec;
    return 0;
  }
  return ENOSYS;
}

static int my_sched_yield(void) {
  if (g_activated_env) {
    g_activated_env->thread_yielded = true;
    return 0;
  }
  return ENOSYS;
}

static int my_sysconf(int name, int *value) {
  if (g_activated_env && name == SYSCONF_TEST_IRT_QUERY) {
    *value = SYSCONF_TEST_VALUE;
    return 0;
  }
  return ENOSYS;
}

static int my_getpid(int *pid) {
  if (g_activated_env) {
    *pid = g_activated_env->pid;
    return 0;
  }
  return ENOSYS;
}

static int my_clock_getres(nacl_irt_clockid_t clock_id, struct timespec *res) {
  if (g_activated_env && clock_id == ENV_CLOCK_ID) {
    res->tv_sec = 1;
    res->tv_nsec = 0;
    return 0;
  }
  return ENOSYS;
}

static int my_clock_gettime(nacl_irt_clockid_t clock_id, struct timespec *tp) {
  if (g_activated_env && clock_id == ENV_CLOCK_ID) {
    tp->tv_sec = g_activated_env->current_time++;
    tp->tv_nsec = 0;
    return 0;
  }
  return ENOSYS;
}

void init_basic_calls_module(void) {
  size_t bytes = nacl_interface_query(NACL_IRT_BASIC_v0_1,
                                      &g_irt_basic, sizeof(g_irt_basic));
  IRT_EXT_ASSERT_MSG(bytes == sizeof(g_irt_basic),
                     "Could not query interface: " NACL_IRT_BASIC_v0_1);

  struct nacl_irt_basic basic_calls = {
    my_exit,
    my_gettod,
    my_clock,
    my_nanosleep,
    my_sched_yield,
    my_sysconf,
  };

  bytes = nacl_interface_ext_supply(NACL_IRT_BASIC_v0_1, &basic_calls,
                                    sizeof(basic_calls));
  IRT_EXT_ASSERT_MSG(bytes == sizeof(basic_calls),
                     "Could not supply interface: " NACL_IRT_BASIC_v0_1);

  struct nacl_irt_dev_getpid getpid_calls = {
    my_getpid,
  };

  bytes = nacl_interface_ext_supply(NACL_IRT_DEV_GETPID_v0_1, &getpid_calls,
                                    sizeof(getpid_calls));
  IRT_EXT_ASSERT_MSG(bytes == sizeof(getpid_calls),
                     "Could not supply interface: " NACL_IRT_DEV_GETPID_v0_1);

  struct nacl_irt_clock clock_calls = {
    my_clock_getres,
    my_clock_gettime,
  };

  bytes = nacl_interface_ext_supply(NACL_IRT_CLOCK_v0_1, &clock_calls,
                                    sizeof(clock_calls));
  IRT_EXT_ASSERT_MSG(bytes == sizeof(clock_calls),
                     "Could not supply interface: " NACL_IRT_CLOCK_v0_1);
}

void init_basic_calls_environment(struct basic_calls_environment *env) {
  *env = g_default_env;
}

void activate_basic_calls_env(struct basic_calls_environment *env) {
  g_activated_env = env;
}

void deactivate_basic_calls_env(void) {
  g_activated_env = NULL;
}
