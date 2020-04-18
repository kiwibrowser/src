/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <windows.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


static void FreeGuardRegions(char *mem_start, size_t addrsp_size) {
  if (NACL_ADDRSPACE_LOWER_GUARD_SIZE != 0) {
    if (!VirtualFree(mem_start - NACL_ADDRSPACE_LOWER_GUARD_SIZE, 0,
                     MEM_RELEASE)) {
      NaClLog(LOG_FATAL,
              "NaClGuardRegionsFree: first VirtualFree() failed, error %d\n",
              GetLastError());
    }
  }
  if (NACL_ADDRSPACE_UPPER_GUARD_SIZE != 0) {
    if (!VirtualFree(mem_start + addrsp_size, 0, MEM_RELEASE)) {
      NaClLog(LOG_FATAL,
              "FreeGuardRegions: second VirtualFree() failed, error %d\n",
              GetLastError());
    }
  }
}

void NaClAddrSpaceFree(struct NaClApp *nap) {
  uintptr_t addrsp_size = (uintptr_t) 1U << nap->addr_bits;

  FreeGuardRegions((char *) nap->mem_start, addrsp_size);

  /*
   * Lock the address space.  This is probably not necessary if we are
   * tearing down, since for tearing down to be safe there should be
   * no other threads using this NaClApp, but we do it for
   * consistency.
   */
  NaClXMutexLock(&nap->mu);

  (*nap->effp->vtbl->UnmapMemory)(nap->effp, nap->mem_start, addrsp_size);

  NaClXMutexUnlock(&nap->mu);
}
