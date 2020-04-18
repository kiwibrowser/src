/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_LOAD_FILE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_LOAD_FILE_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

EXTERN_C_BEGIN

struct NaClApp;

NaClErrorCode NaClAppLoadFileFromFilename(struct NaClApp *nap,
                                          const char *filename) NACL_WUR;

EXTERN_C_END

#endif
