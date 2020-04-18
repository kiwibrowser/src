/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * Simple Perf Counter Layer to be used by the rest of the service run time
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/perf_counter/nacl_perf_counter.h"

#define LAST_IDX(X) (NACL_ARRAY_SIZE(X)-1)

void NaClPerfCounterCtor(struct NaClPerfCounter *sv,
                         const char *app_name) {
  if (NULL == sv) {
    NaClLog(LOG_ERROR, "NaClPerfCounterStart received null pointer\n");
    return;
  };

  memset(sv, 0, sizeof(struct NaClPerfCounter));

  if (NULL == app_name) {
    app_name = "__unknown_app__";
  }

  NACL_ASSERT_IS_ARRAY(sv->app_name);
  strncpy(sv->app_name, app_name, LAST_IDX(sv->app_name));

  /* Being explicit about string termination */
  sv->app_name[LAST_IDX(sv->app_name)] = '\0';

  strncpy(sv->sample_names[0], "__start__", LAST_IDX(sv->sample_names[0]));


  while (0 != NaClGetTimeOfDay(&sv->sample_list[sv->samples])) {
    /* repeat until we get a sample */
  }

  sv->samples++;
}


/*
 * Records the time in sv and  returns its index in sv
 * Note that the first time this routine is called on a sv that was just
 * constructed via NaClPerfCounterCtor(), it will return 1, but that
 * is actually the SECOND sample.
 */
int NaClPerfCounterMark(struct NaClPerfCounter *sv, const char *ev_name) {
  if ((NULL == sv) || (NULL == ev_name)) {
    NaClLog(LOG_ERROR, "NaClPerfCounterMark received null args\n");
    return -1;
  }
  if (sv->samples >= NACL_MAX_PERF_COUNTER_SAMPLES) {
    NaClLog(LOG_ERROR, "NaClPerfCounterMark going beyond buffer size\n");
    return -1;
  }
  while (0 != NaClGetTimeOfDay(&(sv->sample_list[sv->samples]))) {
    /* busy loop until we succeed, damn it */
  }

  /*
   * This relies upon memset() inside NaClPerfCounterCtor() for
   * correctness
   */

  NACL_ASSERT_IS_ARRAY(sv->sample_names[sv->samples]);

  strncpy(sv->sample_names[sv->samples], ev_name,
          LAST_IDX(sv->sample_names[sv->samples]));
  /* Being explicit about string termination */
  sv->sample_names[sv->samples][LAST_IDX(sv->sample_names[sv->samples])] =
    '\0';

  return (sv->samples)++;
}


int64_t NaClPerfCounterInterval(struct NaClPerfCounter *sv,
                                uint32_t a, uint32_t b) {
  if ((NULL != sv) && (a < ((unsigned)sv->samples)) &&
      (b < ((unsigned)sv->samples)) &&
      (sv->samples <= NACL_MAX_PERF_COUNTER_SAMPLES)) {
    uint32_t lo = (a < b)? a : b;
    uint32_t hi = (b < a)? a : b;
    int64_t seconds = (sv->sample_list[hi].nacl_abi_tv_sec -
                       sv->sample_list[lo].nacl_abi_tv_sec);
    int64_t usec = (sv->sample_list[hi].nacl_abi_tv_usec -
                    sv->sample_list[lo].nacl_abi_tv_usec);
    int64_t rtn = seconds * NACL_MICROS_PER_UNIT + usec;

    NaClLog(1, "NaClPerfCounterInterval(%s %s:%s): %"NACL_PRId64" microsecs\n",
            sv->app_name, sv->sample_names[lo], sv->sample_names[hi], rtn);

    return rtn;
  }
  return -1;
}

int64_t NaClPerfCounterIntervalLast(struct NaClPerfCounter *sv) {
  if (NULL != sv) {
    return NaClPerfCounterInterval(sv, sv->samples - 2, sv->samples - 1);
  }
  return -1;
}

int64_t NaClPerfCounterIntervalTotal(struct NaClPerfCounter *sv) {
  if (NULL != sv) {
    return NaClPerfCounterInterval(sv, 0, sv->samples - 1);
  }
  return -1;
}
