/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/osx/mach_thread_map.h"

#include <pthread.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64

static mach_port_t mach_threads[NACL_THREAD_MAX];

uint32_t NaClGetThreadIndexForMachThread(mach_port_t mach_thread) {
  uint32_t nacl_thread_index;

  DCHECK(mach_thread != MACH_PORT_NULL);

  CHECK(NACL_TLS_INDEX_INVALID < NACL_THREAD_MAX);  /* Skip the invalid slot. */
  for (nacl_thread_index = NACL_TLS_INDEX_INVALID + 1;
       nacl_thread_index < NACL_THREAD_MAX;
       ++nacl_thread_index) {
    if (mach_threads[nacl_thread_index] == mach_thread) {
      return nacl_thread_index;
    }
  }

  return NACL_TLS_INDEX_INVALID;
}

void NaClSetCurrentMachThreadForThreadIndex(uint32_t nacl_thread_index) {
  /*
   * This implementation relies on the Mach port for the thread stored by the
   * pthread library, and assumes that the pthread library does not close and
   * re-acquire the Mach port for the thread. If that happens, Mach could
   * theoretically assign the port a different number in the process' port
   * table. This approach avoids having to deal with ownership of the port and
   * the system call overhad to obtain and deallocate it as would be the case
   * with mach_thread_self().
   *
   * When used by the Mach exception handler, this also assumes that the
   * thread port number when received for an exception will match the port
   * stored in the mach_threads table. This is guaranteed by how the kernel
   * coalesces ports in a single port namespace. (A task, or process, is a
   * single port namespace.)
   *
   * An alternative implementation that works on Mac OS X 10.6 or higher is to
   * use pthread_threadid_np() to obtain a thread ID to use as the key for
   * this thread map. Such thread IDs are unique system-wide. An exception
   * handler can find the thread ID for a Mach thread by calling thread_info()
   * with flavor THREAD_IDENTIFIER_INFO. This approach is not used here
   * because of the added system call overhead at exception time.
   */
  mach_port_t mach_thread = pthread_mach_thread_np(pthread_self());
  CHECK(mach_thread != MACH_PORT_NULL);

  DCHECK(nacl_thread_index > NACL_TLS_INDEX_INVALID &&
         nacl_thread_index < NACL_THREAD_MAX);
  DCHECK(mach_threads[nacl_thread_index] == MACH_PORT_NULL);
  DCHECK(NaClGetThreadIndexForMachThread(mach_thread) ==
         NACL_TLS_INDEX_INVALID);

  mach_threads[nacl_thread_index] = mach_thread;
}

void NaClClearMachThreadForThreadIndex(uint32_t nacl_thread_index) {
  mach_port_t mach_thread = mach_threads[nacl_thread_index];

  DCHECK(nacl_thread_index > NACL_TLS_INDEX_INVALID &&
         nacl_thread_index < NACL_THREAD_MAX);
  DCHECK(mach_thread == pthread_mach_thread_np(pthread_self()));

  mach_threads[nacl_thread_index] = MACH_PORT_NULL;

  DCHECK(NaClGetThreadIndexForMachThread(mach_thread) ==
         NACL_TLS_INDEX_INVALID);
}

#endif
