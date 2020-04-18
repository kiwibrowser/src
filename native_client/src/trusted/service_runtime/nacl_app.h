/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  NaCl Application support code.
 */

#ifndef SERVICE_RUNTIME_NACL_APP_H__
#define SERVICE_RUNTIME_NACL_APP_H__

#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

EXTERN_C_BEGIN

struct NaClApp;

NaClErrorCode NaClAppPrepareToLaunch(struct NaClApp     *nap) NACL_WUR;

EXTERN_C_END

#endif
