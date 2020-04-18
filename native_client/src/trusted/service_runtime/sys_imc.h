/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_IMC_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_IMC_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClAbiNaClImcMsgHdr;
struct NaClAppThread;
struct NaClImcMsgHdr;

int32_t NaClSysImcMakeBoundSock(struct NaClAppThread *natp,
                                uint32_t             descs_addr);

int32_t NaClSysImcAccept(struct NaClAppThread  *natp,
                         int                   d);

int32_t NaClSysImcConnect(struct NaClAppThread *natp,
                          int                  d);

int32_t NaClSysImcSendmsg(struct NaClAppThread *natp,
                          int                  d,
                          uint32_t             nanimhp,
                          int                  flags);

int32_t NaClSysImcRecvmsg(struct NaClAppThread *natp,
                          int                  d,
                          uint32_t             nanimhp,
                          int                  flags);

int32_t NaClSysImcMemObjCreate(struct NaClAppThread  *natp,
                               size_t                size);

int32_t NaClSysImcSocketPair(struct NaClAppThread *natp,
                             uint32_t             descs_out);

EXTERN_C_END

#endif
