/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/arch/arm/sel_ldr_arm.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"

static struct NaClMutex gNaClTlsMu;
static int gNaClThreadIdxInUse[NACL_THREAD_MAX];  /* bool */
static size_t const kNumThreads = NACL_ARRAY_SIZE_UNSAFE(gNaClThreadIdxInUse);

/* May be NULL if the current thread does not host a NaClAppThread. */
static THREAD struct NaClThreadContext *nacl_current_thread;

void NaClTlsSetCurrentThread(struct NaClAppThread *natp) {
  nacl_current_thread = &natp->user;
}

struct NaClAppThread *NaClTlsGetCurrentThread(void) {
  return NaClAppThreadFromThreadContext(nacl_current_thread);
}

uint32_t NaClGetThreadIdx(struct NaClAppThread *natp) {
  return natp->user.tls_idx;
}


int NaClTlsInit(void) {
  size_t i;

  NaClLog(2, "NaClTlsInit\n");

  for (i = 0; i < kNumThreads; i++) {
    gNaClThreadIdxInUse[i] = 0;
  }
  if (!NaClMutexCtor(&gNaClTlsMu)) {
    NaClLog(LOG_WARNING,
            "NaClTlsInit: gNaClTlsMu initialization failed\n");
    return 0;
  }

  return 1;
}


void NaClTlsFini(void) {
  NaClLog(2, "NaClTlsFini\n");
  NaClMutexDtor(&gNaClTlsMu);
}

static int NaClThreadIdxAllocate(void) {
  size_t i;

  NaClXMutexLock(&gNaClTlsMu);
  for (i = 1; i < kNumThreads; i++) {
    if (!gNaClThreadIdxInUse[i]) {
      gNaClThreadIdxInUse[i] = 1;
      break;
    }
  }
  NaClXMutexUnlock(&gNaClTlsMu);

  if (kNumThreads != i) {
    return i;
  }

  NaClLog(LOG_ERROR, "NaClThreadIdxAllocate: no more slots for a thread\n");
  return NACL_TLS_INDEX_INVALID;
}


/*
 * Allocation does not mean we can set nacl_current_thread, since we
 * are not that thread.  Setting it must wait until the thread
 * actually launches.
 */
uint32_t NaClTlsAllocate(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);

  return NaClThreadIdxAllocate();
}


void NaClTlsFree(struct NaClAppThread *natp) {
  uint32_t idx = NaClGetThreadIdx(natp);
  NaClLog(2, "NaClTlsFree: old idx %d\n", idx);

  NaClXMutexLock(&gNaClTlsMu);
  gNaClThreadIdxInUse[idx] = 0;
  NaClXMutexUnlock(&gNaClTlsMu);

  natp->user.r9 = 0;
  natp->user.guard_token = 0;
}


void NaClTlsSetTlsValue1(struct NaClAppThread *natp, uint32_t value) {
  natp->user.tls_value1 = value;
}


void NaClTlsSetTlsValue2(struct NaClAppThread *natp, uint32_t value) {
  natp->user.tls_value2 = value;
}


uint32_t NaClTlsGetTlsValue1(struct NaClAppThread *natp) {
  return natp->user.tls_value1;
}


uint32_t NaClTlsGetTlsValue2(struct NaClAppThread *natp) {
  return natp->user.tls_value2;
}
