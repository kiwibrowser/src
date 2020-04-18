/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/performance/perf_test_compat_osx.h"

#include <errno.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/shared/platform/nacl_clock.h"

int clock_gettime(int clk_id, struct timespec *tp) {
  ASSERT_EQ(clk_id, CLOCK_MONOTONIC);

  static int clock_init_rv = NaClClockInit();
  // clock_init_rv is only present for its side effect, static initialization.
  UNREFERENCED_PARAMETER(clock_init_rv);

  nacl_abi_timespec nts;
  int rv = NaClClockGetTime(NACL_CLOCK_MONOTONIC, &nts);
  if (rv != 0) {
    // This assumes that NaCl errno values and Mac OS X errno values are
    // compatible.
    errno = -rv;
    return -1;
  }

  tp->tv_sec = nts.tv_sec;
  tp->tv_nsec = nts.tv_nsec;

  return 0;
}
