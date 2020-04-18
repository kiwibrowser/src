/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sched.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int sched_yield(void) {
  int error = __libnacl_irt_basic.sched_yield();
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
