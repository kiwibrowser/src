/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_DEP_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_DEP_QUALIFY_H_

/*
 * Two of our target architectures (x86-64 and ARM) require that data not be
 * executable for our sandbox to hold.  Every vendor calls this something
 * different.  For the purposes of platform qualification, we use the term
 * 'data execution prevention,' or DEP.
 *
 * This file presents the common interface for DEP-check routines.  The
 * implementations differ greatly across platforms and architectures.  See the
 * notes on each function for details on where implementations live.
 */

#include <stddef.h>
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/*
 * Checks that Data Execution Prevention is working as required by the
 * architecture.  On some architectures this is a no-op.
 *
 * NOTE: the implementation of this function is architecture-specific, and can
 * be found beneath arch/ in this directory.
 */
int NaClCheckDEP(void);

int NaClAttemptToExecuteDataAtAddr(uint8_t *thunk_buffer, size_t size);

/*
 * Attempts to generate and execute some functions in data memory, to verify
 * that Data Execution Prevention is working.  This is only used on platforms
 * where DEP is required.
 *
 * NOTE: the implementation of this function is platform-specific, and can be
 * found in linux/, win/, or osx/ in this directory.
 */
int NaClAttemptToExecuteData(void);

/*
 * A void nullary function.  We generate functions of this type in the heap and
 * stack to prove that we cannot call them.  See NaClGenerateThunk, below.
 */
typedef void (*nacl_void_thunk)(void);

/*
 * Generates an architecture-specific void thunk sequence into the provided
 * buffer.  Returns a pointer to the thunk (which may be offset into the
 * buffer, depending on alignment constraints etc.).
 *
 * If the buffer is too small for the architecture's thunk sequence, returns
 * NULL.  In general the buffer should be more than 4 bytes.
 *
 * NOTE: the implementation of this function is architecture-specific, and can
 * be found beneath arch/ in this directory.
 */
nacl_void_thunk NaClGenerateThunk(uint8_t *buf, size_t buf_size);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_DEP_QUALIFY_H_ */
