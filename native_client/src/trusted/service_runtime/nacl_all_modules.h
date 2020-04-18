/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations here.
 */

#ifndef SERVICE_RUNTIME_NACL_ALL_MODULES_H__
#define SERVICE_RUNTIME_NACL_ALL_MODULES_H__ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

void NaClAllModulesInit(void);
void NaClAllModulesFini(void);

EXTERN_C_END

#endif
