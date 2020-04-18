/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NaCl inter-module communication primitives. */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"

int NaClSocketPair(NaClHandle pair[2]) {
  return imc_socketpair(pair);
}

int NaClClose(NaClHandle handle) {
  return close(handle);
}

NaClHandle NaClCreateMemoryObject(size_t length, int executable) {
  if (executable) {
    return -1;  /* Will never work with NaCl and should never be invoked. */
  }
  return imc_mem_obj_create(length);
}
