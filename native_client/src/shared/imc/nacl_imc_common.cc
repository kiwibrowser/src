/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* NaCl inter-module communication primitives. */

#include "native_client/src/include/build_config.h"

/* Used for UINT32_MAX */
#if !NACL_WINDOWS
# ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
# endif
#include <stdint.h>
#endif

/* TODO(robertm): stdio.h is included for NULL only - find a better way */
#include <stdio.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_log.h"


int NaClMessageSizeIsValid(const NaClMessageHeader *message) {
  size_t cur_bytes = 0;
  static size_t const kMax = ~(uint32_t) 0;
  size_t ix;
  /* we assume that sizeof(uint32_t) <= sizeof(size_t) */

  for (ix = 0; ix < message->iov_length; ++ix) {
    if (kMax - cur_bytes < message->iov[ix].length) {
      return 0;
    }
    cur_bytes += message->iov[ix].length;  /* no overflow is possible */
  }
  return 1;
}
