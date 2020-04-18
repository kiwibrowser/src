/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_FDIO_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_FDIO_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;
struct nacl_abi_stat;

int32_t NaClSysClose(struct NaClAppThread *natp,
                     int                  d);

int32_t NaClSysDup(struct NaClAppThread *natp,
                   int                  oldfd);

int32_t NaClSysDup2(struct NaClAppThread  *natp,
                    int                   oldfd,
                    int                   newfd);

int32_t NaClSysRead(struct NaClAppThread  *natp,
                    int                   d,
                    uint32_t              buf,
                    uint32_t              count);

int32_t NaClSysWrite(struct NaClAppThread *natp,
                     int                  d,
                     uint32_t             buf,
                     uint32_t             count);

int32_t NaClSysLseek(struct NaClAppThread *natp,
                     int                  d,
                     uint32_t             offp,
                     int                  whence);

int32_t NaClSysFstat(struct NaClAppThread *natp,
                     int                  d,
                     uint32_t             nasp);

int32_t NaClSysGetdents(struct NaClAppThread  *natp,
                        int                   d,
                        uint32_t              dirp,
                        size_t                count);

int32_t NaClSysFchdir(struct NaClAppThread *natp,
                      int                  d);

int32_t NaClSysFchmod(struct NaClAppThread *natp,
                      int                  d,
                      int                  mode);

int32_t NaClSysFsync(struct NaClAppThread *natp,
                     int                  d);

int32_t NaClSysFdatasync(struct NaClAppThread *natp,
                         int                  d);

int32_t NaClSysFtruncate(struct NaClAppThread *natp,
                         int                  d,
                         uint32_t             length);

int32_t NaClSysIsatty(struct NaClAppThread *natp,
                      int                  d);

EXTERN_C_END

#endif
