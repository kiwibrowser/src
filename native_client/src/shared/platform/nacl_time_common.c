/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/include/nacl_macros.h"

int64_t NaClGetTimeOfDayMicroseconds(void) {
  struct nacl_abi_timeval tv;
  int ret = NaClGetTimeOfDay(&tv);
  if (ret == 0) {
    return (tv.nacl_abi_tv_sec * NACL_MICROS_PER_UNIT) +
      tv.nacl_abi_tv_usec;
  }

  return -1;
}

