/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nrd_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

int main(void) {
  struct NaClApp app;
  int ret_code;

  NaClNrdAllModulesInit();
  ret_code = NaClAppCtor(&app);
  if (ret_code != 1) {
    printf("init failed\n");
    exit(-1);
  }

  if (app.num_threads != 0) {
    printf("num_threads init failed\n");
    exit(-1);
  }

  NaClRemoveThread(&app, 1);
  NaClNrdAllModulesFini();

  return 0;
}
