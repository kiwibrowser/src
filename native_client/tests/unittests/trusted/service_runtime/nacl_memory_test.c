/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/platform_init.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

void run_check(void) {
  int error;
  void* page;
  int size = NACL_MAP_PAGESIZE;
  if (0 != (error = NaClPageAlloc(&page, size))) {
    errno = -error;
    perror("NaClPageAlloc");
    abort();
  }

  NaClPageFree(page, size);
}

int main(int argc, char* argv[]) {
  int i;
  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  NaClPlatformInit();

  for (i = 0; i < 1000; i++) {
    run_check();
  }

  NaClPlatformFini();

  return 0;
}
