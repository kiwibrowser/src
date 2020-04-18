/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_DEBUG_STUB_DEBUG_STUB_H_
#define NATIVE_CLIENT_DEBUG_STUB_DEBUG_STUB_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

void NaClDebugStubInit(void);
void NaClDebugStubFini(void);

/*
 * Platform-specific init/fini functions
 */
void NaClDebugStubPlatformInit(void);
void NaClDebugStubPlatformFini(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_DEBUG_STUB_DEBUG_STUB_H_ */
