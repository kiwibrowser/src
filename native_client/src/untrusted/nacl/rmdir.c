/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int rmdir(const char *pathname) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_dev_filename.rmdir,
                             __libnacl_irt_dev_filename_init)) {
    return -1;
  }

  int error = __libnacl_irt_dev_filename.rmdir(pathname);
  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}
