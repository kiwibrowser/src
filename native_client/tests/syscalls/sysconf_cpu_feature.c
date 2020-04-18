/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

/*
 * TODO(phosek): Remove once the new constants are added to C library headers.
 */
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

int main(void) {
  long rc;

  rc = sysconf(NACL_ABI__SC_NACL_CPU_FEATURE_X86);
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  assert(rc != -1);
  printf("sysconf(_SC_NACL_CPU_FEATURE_X86) = %ld\n", rc);
#else
  assert(rc == -1);
  assert(errno == EINVAL);
#endif

  return 0;
}
