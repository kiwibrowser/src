/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_FILENAME_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_FILENAME_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;

int32_t NaClSysOpen(struct NaClAppThread  *natp,
                    uint32_t              pathname,
                    int                   flags,
                    int                   mode);

int32_t NaClSysStat(struct NaClAppThread *natp,
                    uint32_t             path,
                    uint32_t             nasp);

int32_t NaClSysMkdir(struct NaClAppThread *natp,
                     uint32_t             path,
                     int                  mode);

int32_t NaClSysRmdir(struct NaClAppThread *natp,
                     uint32_t             path);

int32_t NaClSysChdir(struct NaClAppThread *natp,
                     uint32_t             path);

int32_t NaClSysGetcwd(struct NaClAppThread *natp,
                      uint32_t             buffer,
                      int                  len);

int32_t NaClSysUnlink(struct NaClAppThread *natp,
                      uint32_t             path);

int32_t NaClSysTruncate(struct NaClAppThread *natp,
                        uint32_t             path,
                        uint32_t             length_addr);

int32_t NaClSysLstat(struct NaClAppThread *natp,
                     uint32_t             path,
                     uint32_t             nasp);

int32_t NaClSysLink(struct NaClAppThread *natp,
                    uint32_t              oldpath,
                    uint32_t              newpath);

int32_t NaClSysRename(struct NaClAppThread *natp,
                      uint32_t              oldpath,
                      uint32_t              newpath);

int32_t NaClSysSymlink(struct NaClAppThread *natp,
                       uint32_t             oldpath,
                       uint32_t             newpath);

int32_t NaClSysChmod(struct NaClAppThread *natp,
                     uint32_t             path,
                     nacl_abi_mode_t      mode);

int32_t NaClSysAccess(struct NaClAppThread *natp,
                      uint32_t             path,
                      int                  amode);

int32_t NaClSysReadlink(struct NaClAppThread *natp,
                        uint32_t             path,
                        uint32_t             buffer,
                        uint32_t             buffer_size);

int32_t NaClSysUtimes(struct NaClAppThread *natp,
                      uint32_t             path,
                      uint32_t             times);

EXTERN_C_END

#endif
