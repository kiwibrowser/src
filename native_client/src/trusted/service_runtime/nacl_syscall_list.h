/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SYSCALL_LIST_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SYSCALL_LIST_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * This function registers the default set of syscall handlers in the
 * NaClApp's syscall handler table.
 */
void NaClAppRegisterDefaultSyscalls(struct NaClApp *nap);

EXTERN_C_END

#endif
