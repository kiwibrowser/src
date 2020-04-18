/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_NONSFI_IRT_SIGNAL_HANDLING_H_
#define NATIVE_CLIENT_SRC_PUBLIC_NONSFI_IRT_SIGNAL_HANDLING_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Initialize signal handler for exceptions and async signals at startup
 * before entering sandbox.
 */
void nonsfi_initialize_signal_handler(void);

EXTERN_C_END

#endif
