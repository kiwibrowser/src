/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This module implements the most minimal set of functionality so we can test
 * whether or not basic calls described in nacl_irt_basic are properly
 * intercepted through the irt_extension interface.
 *
 * For time based calls, the current time in the environment will just increment
 * by 1 everytime a time based call is called. Clock ticks will also be in
 * seconds (IE. clock() and gettimeofday() will both return the same value).
 *
 * Most standard libraries have the exit() function declared as noreturn, so
 * testing if this function is hooked into the standard library is a bit more
 * tricky. For this reason, the internal test exit() simply intercepts the
 * status code and converts TEST_EXIT_CODE to exit code 0. The unit testing
 * framework should exit using TEST_EXIT_CODE so the exit status will either
 * fail or be converted to 0.
 */

#ifndef NATIVE_CLIENT_TESTS_IRT_EXT_BASIC_CALLS_H
#define NATIVE_CLIENT_TESTS_IRT_EXT_BASIC_CALLS_H

#include <stdbool.h>
#include <time.h>

#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"

/*
 * The internal test sysconf() can only test and return these values.
 * The value of SYSCONF_TEST_QUERY cannot be any arbitrary thing,
 * because libc's sysconf() does not necessarily pass calls directly
 * through to the IRT interface.  The glibc implementation only calls
 * into the IRT sysconf interface for _SC_NPROCESSORS_ONLN; but the
 * user API (libc) value of _SC_NPROCESSORS_ONLN is not the same as
 * the IRT interface value (NACL_ABI__SC_NPROCESSORS_ONLN).
 */
#define SYSCONF_TEST_LIBC_QUERY _SC_NPROCESSORS_ONLN
#define SYSCONF_TEST_IRT_QUERY NACL_ABI__SC_NPROCESSORS_ONLN
#define SYSCONF_TEST_VALUE 4321

/* Internal test exit code unit testing framework should exit with. */
#define TEST_EXIT_CODE 50

/* Custom clock ID which can be used to reference the environment clock. */
#define ENV_CLOCK_ID 1234

struct basic_calls_environment {
  bool exit_called;
  bool thread_yielded;
  int pid;
  int exit_code;
  time_t current_time;
};

void init_basic_calls_module(void);

void init_basic_calls_environment(struct basic_calls_environment *env);

void activate_basic_calls_env(struct basic_calls_environment *env);
void deactivate_basic_calls_env(void);

#endif /* NATIVE_CLIENT_TESTS_IRT_EXT_BASIC_CALLS_H */
