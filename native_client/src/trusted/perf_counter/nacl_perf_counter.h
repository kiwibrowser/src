/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PERF_COUNTER_NACL_PERF_COUNTER_H
#define NATIVE_CLIENT_SRC_TRUSTED_PERF_COUNTER_NACL_PERF_COUNTER_H 1

/*
 * NaCl performance counter/instrumentation code.
 */

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

EXTERN_C_BEGIN

#define NACL_MAX_PERF_COUNTER_SAMPLES  (16)
#define NACL_MAX_PERF_COUNTER_NAME     (20)
struct NaClPerfCounter {
  char app_name[128]; /* name of the app being run */

  /*
   * It's a basic NaClGetTimeOfDay() based measurement.
   */

  uint32_t samples;
  struct nacl_abi_timeval sample_list[NACL_MAX_PERF_COUNTER_SAMPLES];
  char sample_names[NACL_MAX_PERF_COUNTER_SAMPLES][NACL_MAX_PERF_COUNTER_NAME];

  /*
   * This struct may be extended in the future to include more
   * architecture-specific perf measurements.
   */
};

/*
 * Requires a non-null app name.
 * Starts the measurement count by taking the first time measurement.
 */
extern void NaClPerfCounterCtor(struct NaClPerfCounter *sv,
                                const char *app_name);

/*
 * Adds one more time measurement sample.
 * Returns the index of the time sample being made.
 * Requires a non-null short string to tag the event with a name
 */
extern int NaClPerfCounterMark(struct NaClPerfCounter *sv,
                               const char *ev_name);

/* Returns the time spent between two sampling points, in microseconds */
extern int64_t NaClPerfCounterInterval(struct NaClPerfCounter *sv,
                                       uint32_t sample1,
                                       uint32_t sample2);

/* Returns the time spent between the last sampling points, in microseconds */
extern int64_t NaClPerfCounterIntervalLast(struct NaClPerfCounter *sv);

/* Returns the time spent between all sampling points, in microseconds */
extern int64_t NaClPerfCounterIntervalTotal(struct NaClPerfCounter *sv);

/* Prefix for important events and app_names */
#define NACL_PERF_IMPORTANT_PREFIX "*"


EXTERN_C_END

#endif
