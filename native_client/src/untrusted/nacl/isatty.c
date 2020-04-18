/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int isatty(int fd) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_dev_fdio.isatty,
                             __libnacl_irt_dev_fdio_init)) {
    return 0;
  }

  int result;
  int error = __libnacl_irt_dev_fdio.isatty(fd, &result);
  if (error) {
    errno = error;
    return 0;
  }

  return result;
}
