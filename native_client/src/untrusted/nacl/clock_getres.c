/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <time.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int clock_getres(clockid_t clock_id, struct timespec *res) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_clock.clock_getres,
                             __libnacl_irt_clock_init)) {
    return -1;
  }

  int error = __libnacl_irt_clock.clock_getres(clock_id, res);
  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}
