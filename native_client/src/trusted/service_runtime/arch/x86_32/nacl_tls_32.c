/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


int NaClTlsInit(void) {
  return NaClLdtInit();
}


void NaClTlsFini(void) {
  NaClLdtFini();
}


uint32_t NaClTlsAllocate(struct NaClAppThread *natp) {
  /*
   * Initialize these so that untrusted code cannot read uninitialized
   * values from the %gs segment.
   */
  natp->user.gs_segment.new_prog_ctr = 0;
  natp->user.gs_segment.new_ecx = 0;

  return (uint32_t) NaClLdtAllocateByteSelector(NACL_LDT_DESCRIPTOR_DATA,
                                                /* read_exec_only= */ 1,
                                                &natp->user.gs_segment,
                                                sizeof(natp->user.gs_segment));
}


void NaClTlsFree(struct NaClAppThread *natp) {
  NaClLdtDeleteSelector(natp->user.gs);
}


void NaClTlsSetTlsValue1(struct NaClAppThread *natp, uint32_t value) {
  natp->user.gs_segment.tls_value1 = value;
}


void NaClTlsSetTlsValue2(struct NaClAppThread *natp, uint32_t value) {
  natp->user.gs_segment.tls_value2 = value;
}


uint32_t NaClTlsGetTlsValue1(struct NaClAppThread *natp) {
  return natp->user.gs_segment.tls_value1;
}


uint32_t NaClTlsGetTlsValue2(struct NaClAppThread *natp) {
  return natp->user.gs_segment.tls_value2;
}


uint32_t NaClGetThreadIdx(struct NaClAppThread *natp) {
  return natp->user.gs >> 3;
}

#if NACL_LINUX

/*
 * This TLS variable mirrors nacl_current_thread in the x86-64 sandbox,
 * except that, on x86-32, we only use it for getting the identity of
 * the interrupted thread in a signal handler in the Linux
 * implementation of thread suspension.
 *
 * We should not enable this code on Windows because TLS variables do
 * not work inside dynamically-loaded DLLs -- such as chrome.dll -- on
 * Windows XP.
 */
static THREAD struct NaClThreadContext *nacl_current_thread;

void NaClTlsSetCurrentThread(struct NaClAppThread *natp) {
  nacl_current_thread = &natp->user;
}

struct NaClAppThread *NaClTlsGetCurrentThread(void) {
  return NaClAppThreadFromThreadContext(nacl_current_thread);
}

#else

/*
 * This is a NOOP, since TLS (or TSD) is not used to keep the thread
 * index on the x86-32.  We use segmentation (%gs) to provide access
 * to the per-thread data, and the segment selector itself tells us
 * the thread's identity.
 */
void NaClTlsSetCurrentThread(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
}

#endif
