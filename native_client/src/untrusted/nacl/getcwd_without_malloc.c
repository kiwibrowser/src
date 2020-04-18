/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "native_client/src/untrusted/nacl/getcwd.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"

char *__getcwd_without_malloc(char *buf, size_t size) {
  if (!__libnacl_irt_init_fn(&__libnacl_irt_dev_filename.getcwd,
                             __libnacl_irt_dev_filename_init)) {
    return NULL;
  }

  int error = __libnacl_irt_dev_filename.getcwd(buf, size);
  if (error) {
    errno = error;
    return NULL;
  }

  return buf;
}
