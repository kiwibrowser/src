/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_clock.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"

void NaClPlatformInit(void) {
  NaClLogModuleInit();
  NaClTimeInit();
  if (!NaClClockInit()) {
    NaClLog(LOG_FATAL, "NaClPlatformInit: NaClClockInit failed\n");
  }
  NaClSecureRngModuleInit();
  NaClGlobalSecureRngInit();
}

void NaClPlatformFini(void) {
  NaClGlobalSecureRngFini();
  NaClSecureRngModuleFini();
  NaClClockFini();
  NaClTimeFini();
  NaClLogModuleFini();
}
