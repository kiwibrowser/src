/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/stat.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int stat(const char *file, struct stat *st) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_dev_filename.stat,
                             __libnacl_irt_dev_filename_init)) {
    return -1;
  }

  int error = __libnacl_irt_dev_filename.stat(file, st);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
