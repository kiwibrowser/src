/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/untrusted/irt/irt.h"

int main(void) {
  struct nacl_irt_thread ti;
  if (0 == nacl_interface_query(NACL_IRT_THREAD_v0_1, &ti, sizeof ti)) {
    fprintf(stderr, "IRT hook is not available\n");
    return 1;
  }

  return 0;
}
