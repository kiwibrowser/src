/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_ERROR_LOG_HOOK_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_ERROR_LOG_HOOK_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

void NaClErrorLogHookInit(void (*hook)(void *state,
                                       char *buf,
                                       size_t buf_bytes),
                          void *state);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_ERROR_LOG_HOOK_H_ */
