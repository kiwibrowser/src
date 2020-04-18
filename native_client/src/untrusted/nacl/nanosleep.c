/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <time.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int nanosleep(const struct timespec *req, struct timespec *rem) {
  int error = __libnacl_irt_basic.nanosleep(req, rem);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
