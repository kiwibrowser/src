/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, list_mappings system call.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_LIST_MAPPINGS_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_LIST_MAPPINGS_H__ 1

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"

EXTERN_C_BEGIN

struct NaClAppThread;
struct NaClMemMappingInfo;

int32_t NaClSysListMappings(struct NaClAppThread *natp,
                            uint32_t             regions,
                            uint32_t             count);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_LIST_MAPPINGS_H__ */
