/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <time.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

void __libnacl_irt_clock_init(void) {
  __libnacl_irt_query(NACL_IRT_CLOCK_v0_1,
                      &__libnacl_irt_clock, sizeof(__libnacl_irt_clock));
}

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_clock.clock_gettime,
                             __libnacl_irt_clock_init)) {
    return -1;
  }

  int error = __libnacl_irt_clock.clock_gettime(clock_id, tp);
  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}
