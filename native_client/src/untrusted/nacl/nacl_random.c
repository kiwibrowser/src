/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

static struct nacl_irt_random random_interface = { NULL };

int nacl_secure_random_init(void) {
  return 0;  /* Success */
}

static void random_init(void) {
  nacl_interface_query(NACL_IRT_RANDOM_v0_1, &random_interface,
                       sizeof(random_interface));
}

int nacl_secure_random(void *dest, size_t bytes, size_t *bytes_written) {
  if (!__libnacl_irt_init_fn(&random_interface.get_random_bytes,
                             random_init)) {
    return ENOSYS;
  }
  return random_interface.get_random_bytes(dest, bytes, bytes_written);
}
