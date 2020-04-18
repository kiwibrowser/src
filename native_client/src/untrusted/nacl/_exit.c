/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

void _exit(int status) {
  __libnacl_irt_basic.exit(status);
  __builtin_trap();
}
