/*
 * Copyright 2006 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl run time.
 */

#include <fcntl.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/desc/nacl_desc_io.h"

#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

/*
 * No segments: (currently) no-op.
 */
NaClErrorCode NaClAppPrepareToLaunch(struct NaClApp     *nap) {
  UNREFERENCED_PARAMETER(nap);

  return LOAD_OK;
}
