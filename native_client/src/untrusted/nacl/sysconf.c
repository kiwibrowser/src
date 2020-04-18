/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

long int sysconf(int name) {
  int value;
  int error = __libnacl_irt_basic.sysconf(name, &value);
  if (error) {
    errno = error;
    return -1L;
  }
  return value;
}
