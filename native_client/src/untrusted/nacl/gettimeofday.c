/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/time.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int gettimeofday(struct timeval *tv, void *tz) {
  int error = __libnacl_irt_basic.gettod(tv);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
