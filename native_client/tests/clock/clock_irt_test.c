/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "native_client/src/untrusted/irt/irt.h"

#define DEFERED_STRINGIFY(symbol) #symbol
#define SHOW(symbol)                                  \
  do {                                                \
    printf("Current definition for %s is \"%s\".\n",  \
           #symbol, DEFERED_STRINGIFY(symbol));       \
  } while (0)

void ShowCurrentDefinitions(void) {
  SHOW(CLOCK_REALTIME);
  SHOW(CLOCK_MONOTONIC);
  SHOW(CLOCK_PROCESS_CPUTIME_ID);
  SHOW(CLOCK_THREAD_CPUTIME_ID);
}
/*
 * We would normally just use the values of CLOCK_REALTIME, etc from
 * <time.h> as required by POSIX.  Unfortunately, the POSIX-required
 * names CLOCK_REALTIME are different in the newlib toolchain from the
 * values used by the service runtime -- and from the values used in
 * the glibc toolchain!  -- so we have to have the following hack
 * until the toolchains are fixed (and DEPS rolled) to make it all
 * consistent.  (Newlib defines CLOCK_REALTIME but none of the
 * others.)
 *
 * TODO(bsy): remove out when the toolchains have correct definitions
 * for these preprocessor symbols.
 */
#undef CLOCK_REALTIME
#undef CLOCK_MONOTONIC
#undef CLOCK_PROCESS_CPUTIME_ID
#undef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_REALTIME           (0)
#define CLOCK_MONOTONIC          (1)
#define CLOCK_PROCESS_CPUTIME_ID (2)
#define CLOCK_THREAD_CPUTIME_ID  (3)

struct timespec;

/*
 * Basic functionality test: the syscalls are present.
 */
int TimeTest(int (*func)(nacl_irt_clockid_t clk_id, struct timespec *ts),
             nacl_irt_clockid_t clk_id,
             char const *error_string,
             char const *success_name) {
  struct timespec       ts;

  if (0 != (*func)(clk_id, &ts)) {
    fprintf(stderr, "%s\n", error_string);
    return 1;
  }
  printf("%30s: %lld.%09lu\n", success_name,
         (int64_t) ts.tv_sec, (unsigned long) ts.tv_nsec);
  return 0;
}

int main(void) {
  struct nacl_irt_clock ti;
  int                   errs = 0;

  ShowCurrentDefinitions();

  if (0 == nacl_interface_query(NACL_IRT_CLOCK_v0_1, &ti, sizeof(ti))) {
    fprintf(stderr, "IRT hook is not available\n");
    return 1;
  }

  errs += TimeTest(ti.clock_getres, CLOCK_REALTIME,
                   "clock_getres on realtime clock failed",
                   "Realtime clock resolution");
  errs += TimeTest(ti.clock_getres, CLOCK_MONOTONIC,
                   "clock_getres on monotonic clock failed",
                   "Monotonic clock resolution");
  errs += TimeTest(ti.clock_getres, CLOCK_PROCESS_CPUTIME_ID,
                   "clock_getres on process CPU-time clock failed",
                   "Process CPU-time clock resolution");
  errs += TimeTest(ti.clock_getres, CLOCK_THREAD_CPUTIME_ID,
                   "clock_getres on thread CPU-time clock failed",
                   "Thread CPU-time clock resolution");
  errs += TimeTest(ti.clock_gettime, CLOCK_REALTIME,
                   "clock_gettime on realtime clock failed",
                   "Realtime clock value");
  errs += TimeTest(ti.clock_gettime, CLOCK_MONOTONIC,
                   "clock_gettime on monotonic clock failed",
                   "Monotonic clock value");
  errs += TimeTest(ti.clock_gettime, CLOCK_PROCESS_CPUTIME_ID,
                   "clock_gettime on process CPU-time clock failed",
                   "Process CPU-time clock value");
  errs += TimeTest(ti.clock_gettime, CLOCK_THREAD_CPUTIME_ID,
                   "clock_gettime on thread CPU-time clock failed",
                   "Thread CPU-time clock value");
  return errs;
}
