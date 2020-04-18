/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"

/*
 * The launcher passes --reserved_at_zero=0xXXXXXXXXXXXXXXXX.
 * nacl_helper_bootstrap replaces the Xs wih the size of the
 * prereserved sandbox memory.
 */
void NaClHandleReservedAtZero(const char *switch_value) {
  char *endp = NULL;
  size_t prereserved_sandbox_size = (size_t) strtoul(switch_value, &endp, 0);
  if (*endp != '\0') {
    NaClLog(LOG_FATAL, "NaClHandleReservedAtZero: Could not parse"
            " reserved_at_zero argument value of %s\n", switch_value);
  }
  g_prereserved_sandbox_size = prereserved_sandbox_size;
}
