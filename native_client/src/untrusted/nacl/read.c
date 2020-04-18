/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int read(int desc, void *buf, size_t count) {
  size_t nread;
  int error = __libnacl_irt_fdio.read(desc, buf, count, &nread);
  if (error) {
    errno = error;
    return -1;
  }
  return nread;
}
