/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

off_t lseek(int desc, off_t offset, int whence) {
  off_t result;
  int error = __libnacl_irt_fdio.seek(desc, offset, whence, &result);
  if (error) {
    errno = error;
    return (off_t) -1;
  }
  return result;
}
