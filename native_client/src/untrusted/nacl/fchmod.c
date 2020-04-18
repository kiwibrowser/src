/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int fchmod(int fd, mode_t mode) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_dev_fdio.fchmod,
                             __libnacl_irt_dev_fdio_init)) {
    return -1;
  }

  int error = __libnacl_irt_dev_fdio.fchmod(fd, mode);
  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}
