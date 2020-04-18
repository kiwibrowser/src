/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SERVICE_RUNTIME_NACL_SYSCALL_HANDLERS_H__
#define SERVICE_RUNTIME_NACL_SYSCALL_HANDLERS_H__ 1

#include "native_client/src/include/nacl_base.h"

struct NaClAppThread;

struct NaClSyscallTableEntry {
  int32_t (*handler)(struct NaClAppThread *natp);
};

#endif
