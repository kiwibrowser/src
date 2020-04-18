/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations needed by
 * the NRD transfer protocol code here.
 */

#include "native_client/src/trusted/desc/nrd_all_modules.h"
#include "native_client/src/trusted/desc/nacl_desc_invalid.h"
#include "native_client/src/shared/platform/platform_init.h"

void NaClNrdAllModulesInit(void) {
  /*
   * NaClLogModuleInit examines NACLLOG and NACLVERBOSITY environment
   * variables.
   */
  NaClPlatformInit();
  NaClDescInvalidInit();
}

void NaClNrdAllModulesFini(void) {
  NaClDescInvalidFini();
  NaClPlatformFini();
}
