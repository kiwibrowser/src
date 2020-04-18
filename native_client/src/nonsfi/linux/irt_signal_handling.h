/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_NONSFI_LINUX_IRT_SIGNAL_HANDLING_H_
#define NATIVE_CLIENT_SRC_NONSFI_LINUX_IRT_SIGNAL_HANDLING_H_ 1

#include "native_client/src/untrusted/irt/irt.h"

EXTERN_C_BEGIN

/*
 * See documentation for nacl_irt_async_signal_handler members in
 * src/untrusted/irt/irt.h
 */
int nacl_async_signal_set_handler(NaClIrtAsyncSignalHandler handler);
int nacl_async_signal_send_async_signal(nacl_irt_tid_t tid);

EXTERN_C_END

#endif
