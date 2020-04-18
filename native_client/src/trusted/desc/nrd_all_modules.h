/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations needed by
 * the NRD transfer protocol code here.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_ALL_MODULES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_ALL_MODULES_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

void NaClNrdAllModulesInit(void);

void NaClNrdAllModulesFini(void);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_ALL_MODULES_H_ */
