/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/types.h>

#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"

int main(void) {
  typedef int (*syswrite_t)(int, char const *, size_t);
  syswrite_t syswrite = (syswrite_t) (0x10000 + NACL_sys_write * 0x20);

  (void) (*syswrite)(1, "Hello world\n", 12);
  return 0;
}
