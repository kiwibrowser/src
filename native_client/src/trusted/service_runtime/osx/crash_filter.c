/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/osx/crash_filter.h"

#include <inttypes.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/osx/mach_thread_map.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"


#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86

int NaClMachThreadIsInUntrusted(mach_port_t thread_port) {
  x86_thread_state_t state;
  thread_state_t statep = (thread_state_t) &state;
  mach_msg_type_number_t size = x86_THREAD_STATE_COUNT;
  kern_return_t kr;
  uint32_t nacl_thread_index;

  kr = thread_get_state(thread_port, x86_THREAD_STATE, statep, &size);
  if (kr != KERN_SUCCESS) {
    NaClLog(LOG_FATAL, "NaClMachThreadIsInUntrusted: "
            "thread_get_state() failed with error %i\n", kr);
  }
  CHECK(kr == KERN_SUCCESS);

#if NACL_BUILD_SUBARCH == 32
  CHECK(state.tsh.flavor == x86_THREAD_STATE32);
  nacl_thread_index = state.uts.ts32.__gs >> 3;
#elif NACL_BUILD_SUBARCH == 64
  nacl_thread_index = NaClGetThreadIndexForMachThread(thread_port);

  /*
   * If the thread isn't known to Native Client, it's not untrusted (at least
   * not by Native Client.)
   */
  if (nacl_thread_index == NACL_TLS_INDEX_INVALID) {
    return 0;
  }
#endif

  return NaClMachThreadStateIsInUntrusted(&state, nacl_thread_index);
}

int NaClMachThreadStateIsInUntrusted(x86_thread_state_t *state,
                                     size_t nacl_thread_index) {
#if NACL_BUILD_SUBARCH == 32

  uint16_t global_cs;

  UNREFERENCED_PARAMETER(nacl_thread_index);

  CHECK(state->tsh.flavor == x86_THREAD_STATE32);

  global_cs = NaClGetGlobalCs();

  /*
   * If global_cs is 0 (which is not a usable segment selector), the
   * sandbox has not been initialised yet, so there can be no untrusted
   * code running.
   */
  if (global_cs == 0) {
    return 0;
  }

  return state->uts.ts32.__cs != global_cs;

#elif NACL_BUILD_SUBARCH == 64

  struct NaClAppThread *natp;

  CHECK(state->tsh.flavor == x86_THREAD_STATE64);

  natp = NaClAppThreadGetFromIndex(nacl_thread_index);
  return NaClIsUserAddr(natp->nap, state->uts.ts64.__rip);

#endif  /* NACL_BUILD_SUBARCH */
}

#endif  /* NACL_ARCH(NACL_BUILD_ARCH) */
