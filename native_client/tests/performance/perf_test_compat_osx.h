/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_PERFORMANCE_PERF_TEST_COMPAT_OSX_H_
#define NATIVE_CLIENT_TESTS_PERFORMANCE_PERF_TEST_COMPAT_OSX_H_

#include "native_client/src/include/build_config.h"

#if NACL_OSX

#include <time.h>

// A compatibility implementation of clock_gettime, which does not exist
// natively on Mac OS X, built on mach_absolute_time. This only supports
// CLOCK_MONOTONIC.
#define CLOCK_MONOTONIC 1
int clock_gettime(int clk_id, struct timespec *tp);

#endif  // NACL_OSX

#endif  // NATIVE_CLIENT_TESTS_PERFORMANCE_PERF_TEST_COMPAT_OSX_H_
