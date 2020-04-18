/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/nacl_all_modules.h"
#include "native_client/src/trusted/service_runtime/nacl_resource.h"

#define MEGABYTE (1000000)

int main(void) {
  struct NaClApp ap;
  struct NaClApp *nap = &ap;
  struct NaClDesc *d_null;

  NaClAllModulesInit();

  if (!NaClAppCtor(nap)) {
    NaClLog(LOG_FATAL, "NaClAppCtor failed\n");
  }
  if (!NaClResourceNaClAppInit(&nap->resources, nap)) {
    NaClLog(LOG_FATAL, "NaClResourceNaClAppInit failed\n");
  }
  d_null = NaClResourceOpen((struct NaClResource *) &nap->resources,
                            "/dev/null",
                            NACL_ABI_O_RDWR,
                            0777);
  if (NULL == d_null) {
    NaClLog(LOG_FATAL, "NaClResourceOpen failed\n");
  }

  if (NACL_VTBL(NaClDesc, d_null)->typeTag != NACL_DESC_NULL) {
    NaClLog(LOG_FATAL, "NaClResourceOpen did not return a null descriptor\n");
  }
  if ((*NACL_VTBL(NaClDesc, d_null)->Write)(d_null, NULL, MEGABYTE)
      != MEGABYTE) {
    NaClLog(LOG_FATAL, "Write failed\n");
  }
  if ((*NACL_VTBL(NaClDesc, d_null)->Read)(d_null, NULL, MEGABYTE)
      != 0) {
    NaClLog(LOG_FATAL, "Read failed\n");
  }

  printf("Passed.\n");
  return 0;
}
