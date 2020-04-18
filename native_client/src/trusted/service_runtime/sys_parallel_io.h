/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SYS_PARALLEL_IO_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SYS_PARALLEL_IO_H__

#include "native_client/src/include/portability.h"

struct NaClAppThread;

int32_t NaClSysPRead(struct NaClAppThread *natp,
                     int32_t desc,
                     uint32_t usr_addr,
                     uint32_t buffer_bytes,
                     uint32_t offset_addr);

int32_t NaClSysPWrite(struct NaClAppThread *natp,
                      int32_t desc,
                      uint32_t usr_addr,
                      uint32_t buffer_bytes,
                      uint32_t offset_addr);

#endif
