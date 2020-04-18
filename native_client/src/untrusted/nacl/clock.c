/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <time.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

clock_t clock(void) {
  nacl_irt_clock_t result;
  int error = __libnacl_irt_basic.clock(&result);
  if (error) {
    errno = error;
    return (clock_t) -1;
  }
  return (clock_t) result;
}
