/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_ERROR_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_ERROR_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * NaClGetLastErrorString() returns a string describing the last-error or errno
 * of the calling thread. It writes to the user-supplied buffer 'buf' of length
 * 'length', and returns 0 on success or -1 on failure.
 */

int NaClGetLastErrorString(char* buffer, size_t length);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_ERROR_H_ */
