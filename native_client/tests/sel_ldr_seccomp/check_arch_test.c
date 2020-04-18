/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/seccomp_bpf/seccomp_bpf.h"

int main(void) {
  CHECK(0 == NaClInstallBpfFilter());

  /*
   * calling x86-32's __NR_exit
   */
  __asm__ volatile("int $0x80" : :
                   "a"(1) /* %eax, syscall number */,
                   "b"(123) /* %ebx, syscall argument 1 */);

  return 0;
}
