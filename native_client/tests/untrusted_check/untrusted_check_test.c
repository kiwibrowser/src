/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"

/*
 * NB: platform_init is used in trusted code; unfortunately, the
 * untrusted version of the platform library is a subset of the
 * trusted platform library, and it is not currently possible to use
 * platform_init.h
 *
 * See http://code.google.com/p/nativeclient/issues/detail?id=1358 for
 * a sketch of what needs to be done to eliminate the need to invoke
 * NaClLogModuleInit explictly.
 *
 * #include "native_client/src/shared/platform/platform_init.h"
 */

int main(void) {
  /* NaClPlatformInit(); -- not available in untrusted platform lib */
  NaClLogModuleInit();
  NaClLog(LOG_INFO, "Log output\n");
  CHECK(0);
  return 0;
}
