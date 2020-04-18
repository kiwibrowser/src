/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_MEMORY_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_MEMORY_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;

int32_t NaClSysBrk(struct NaClAppThread *natp,
                   uintptr_t            new_break);

int32_t NaClSysMmap(struct NaClAppThread  *natp,
                    uint32_t              start,
                    size_t                length,
                    int                   prot,
                    int                   flags,
                    int                   d,
                    uint32_t              offp);

int32_t NaClSysMmapIntern(struct NaClApp  *nap,
                          void            *start,
                          size_t          length,
                          int             prot,
                          int             flags,
                          int             d,
                          nacl_abi_off_t  offset);

int32_t NaClSysMprotectInternal(struct NaClApp  *nap,
                                uint32_t        start,
                                size_t          length,
                                int             prot);

int32_t NaClSysMprotect(struct NaClAppThread  *natp,
                        uint32_t              start,
                        size_t                length,
                        int                   prot);

int32_t NaClSysMunmap(struct NaClAppThread  *natp,
                      uint32_t              start,
                      uint32_t              length);

EXTERN_C_END

#endif
