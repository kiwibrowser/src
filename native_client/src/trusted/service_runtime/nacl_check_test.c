/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_log_intern.h"

void MyAbort(void) {
  exit(17);
}

int main(int ac,
         char **av) {
  int opt;


  NaClLogModuleInit();
  gNaClLogAbortBehavior = MyAbort;

  while (-1 != (opt = getopt(ac, av, "cds:CD"))) {
    switch (opt) {
      case 'c':
        CHECK(0);
        break;
      case 'd':
        DCHECK(0);
        break;
      case 's':
        NaClCheckSetDebugMode(strtol(optarg, (char **) 0, 0));
        break;
      case 'C':
        CHECK(1);
        break;
      case 'D':
        DCHECK(1);
        break;
    }
  }
  NaClLogModuleFini();
  return 0;
}
