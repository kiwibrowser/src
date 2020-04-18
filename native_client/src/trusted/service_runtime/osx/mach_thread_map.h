/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_OSX_MACH_THREAD_MAP_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_OSX_MACH_THREAD_MAP_H_ 1

#include "native_client/src/include/build_config.h"

#if NACL_OSX

#include <mach/mach.h>
#include <stdint.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"

EXTERN_C_BEGIN

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64

/*
 * Retrieves the Native Client thread index for a given Mach thread, or
 * NACL_TLS_INDEX_INVALID if the Mach thread is unknown to Native Client.
 */
uint32_t NaClGetThreadIndexForMachThread(mach_port_t mach_thread);

/*
 * Establishes a mapping from the currently executing Mach thread to the
 * given Native Client thread index.
 */
void NaClSetCurrentMachThreadForThreadIndex(uint32_t nacl_thread_index);

/*
 * Clears any existing mapping to any Mach thread for the given Native Client
 * thread index. A thread's mapping must be cleared by itself, not by another
 * thread.
 */
void NaClClearMachThreadForThreadIndex(uint32_t nacl_thread_index);

#else

/*
 * NaClGetThreadIndexForMachThread is not implemented for x86-32, although it
 * it could be implemented by using thread_get_state to obtain the thread's
 * state and returning %gs >> 3. It's left unimplemented because the only
 * prospective call sites already have access to the thread state and thus %gs
 * themselves, and directing them through this function would add system call
 * overhead for a redundant thread_get_state.
 */

static INLINE void NaClSetCurrentMachThreadForThreadIndex(
    uint32_t nacl_thread_index) {
  /* No-op for x86-32. */
  UNREFERENCED_PARAMETER(nacl_thread_index);
}

static INLINE void NaClClearMachThreadForThreadIndex(
    uint32_t nacl_thread_index) {
  /* No-op for x86-32. */
  UNREFERENCED_PARAMETER(nacl_thread_index);
}

#endif

EXTERN_C_END

#endif  /* NACL_OSX */

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_OSX_MACH_THREAD_MAP_H_ */
