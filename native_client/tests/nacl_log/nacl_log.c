/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
   A simple test that the NaClLog functionality is working.
*/

#include <stdio.h>

#include "native_client/src/shared/platform/nacl_log.h"

void hello_world(void) {
  NaClLog(LOG_INFO, "Hello, World!\n");
}

int main(int argc, char* argv[]) {
  NaClLogModuleInit();
  hello_world();
  NaClLogModuleFini();
  return 0;
}
