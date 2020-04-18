/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MAIN_COMMON_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MAIN_COMMON_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClDesc;
struct NaClValidationMetadata;

NaClErrorCode NaClMainLoadIrt(struct NaClApp *nap, struct NaClDesc *nd,
                              struct NaClValidationMetadata *metadata);

const char ** NaClGetEnviron(void);

EXTERN_C_END

#endif
