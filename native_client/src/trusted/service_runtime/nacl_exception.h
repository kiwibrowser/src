/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_EXCEPTION_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_EXCEPTION_H__ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"

struct NaClExceptionContext;

/*
 * This is the stack frame that we write to the untrusted stack in
 * order to call an untrusted hardware exception handler.
 */
struct NaClExceptionFrame {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  nacl_reg_t return_addr;
# if NACL_BUILD_SUBARCH == 32
  uint32_t context_ptr;  /* Function argument */
# endif
#endif
  struct NaClExceptionContext context;
  struct NaClExceptionPortableContext portable;
};

#endif
