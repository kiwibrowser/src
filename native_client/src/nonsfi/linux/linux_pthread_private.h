/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_NONSFI_LINUX_LINUX_PTHREAD_PRIVATE_H_
#define NATIVE_CLIENT_SRC_NONSFI_LINUX_LINUX_PTHREAD_PRIVATE_H_ 1

#include <stdint.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/untrusted/irt/irt.h"

EXTERN_C_BEGIN

/*
 * Creates a new thread by calling clone(). This function does use the |stack|
 * parameter and the thread's entrypoint is |start_func| with |thread_ptr| as
 * its parameter.
 */
int nacl_user_thread_create(void *(*start_func)(void *), void *stack,
                            void *thread_ptr, nacl_irt_tid_t *child_tid);

/*
 * Exits a thread started by |nacl_user_thread_create|. Threads started with
 * pthread_create() should terminate by calling pthread_exit() instead.
 */
void nacl_user_thread_exit(int32_t *stack_flag);

EXTERN_C_END

#endif
