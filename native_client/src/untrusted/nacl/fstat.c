/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/stat.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int fstat(int fd, struct stat *st) {
  int error = __libnacl_irt_fdio.fstat(fd, st);
  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}
