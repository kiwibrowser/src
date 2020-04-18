/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int write(int desc, const void *buf, size_t count) {
  size_t nwrote;
  int error = __libnacl_irt_fdio.write(desc, buf, count, &nwrote);
  if (error) {
    errno = error;
    return -1;
  }
  return nwrote;
}
