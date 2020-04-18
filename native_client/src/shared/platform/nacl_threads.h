/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime threads abstraction layer.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_THREADS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_THREADS_H_

/*
 * We cannot include this header file from an installation that does not
 * have the native_client source tree.
 * TODO(sehr): use export_header.py to copy these files out.
 */
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

#if NACL_LINUX || NACL_OSX || defined(__native_client__)
# include "native_client/src/shared/platform/posix/nacl_threads_types.h"
#elif NACL_WINDOWS
/*
 * Needed for WINAPI.
 */
# include "native_client/src/shared/platform/win/nacl_threads_types.h"
#else
# error "thread abstraction not defined for target OS"
#endif

/*
 * We provide two simple, portable thread creation interfaces:
 *
 * Joinable threads, created with NaClThreadCreateJoinable().  These
 * must eventually be waited for using NaClThreadJoin() or the thread
 * handle will leak.
 *
 * Non-joinable (detached) threads, created with NaClThreadCtor().
 * These cannot be waited for.
 * TODO(mseaborn): The corresponding NaClThreadDtor() needs to be
 * called at some point to prevent a handle leak, but it should
 * probably be merged into NaClThreadCtor().  Calling it immediately
 * after NaClThreadCtor() should be OK.
 *
 * Returns a non-zero value if a thread is successfully created,
 * and returns zero if thread creation failed.
 */

int NaClThreadCtor(struct NaClThread  *ntp,
                   void (WINAPI *start_fn)(void *),
                   void *state,
                   size_t stack_size) NACL_WUR;
void NaClThreadDtor(struct NaClThread *ntp);

int NaClThreadCreateJoinable(struct NaClThread  *ntp,
                             void (WINAPI *start_fn)(void *),
                             void *state,
                             size_t stack_size) NACL_WUR;
void NaClThreadJoin(struct NaClThread *ntp);

/*
 * NaClThreadExit() terminates the current thread.
 */
void NaClThreadExit(void);

uint32_t NaClThreadId(void);

/* NaClThreadYield() makes the calling thread yield the processor. */
void NaClThreadYield(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_THREADS_H_ */
