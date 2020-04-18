/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_OSX_MACH_EXCEPTION_HANDLER_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_OSX_MACH_EXCEPTION_HANDLER_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Start a NaCl specific Mach exception handler thread to intercept and
 * redirect exceptions.
 * On success returns TRUE otherwise FALSE.
 */
int NaClInterceptMachExceptions(void);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SERVICE_RUNTIME_OSX_MACH_EXCEPTION_HANDLER_H_ */
