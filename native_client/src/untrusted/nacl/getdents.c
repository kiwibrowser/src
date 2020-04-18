/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int getdents(int desc, struct dirent *buf, size_t count) {
  size_t nread;
  int error = __libnacl_irt_fdio.getdents(desc, buf, count, &nread);
  if (error) {
    errno = error;
    return -1;
  }
  return nread;
}
