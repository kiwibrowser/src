/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* NaCl inter-module communication primitives. */

#include <windows.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This function is a no-op on Windows because there is no need to
 * override the Windows definition of NaClCreateMemoryObject(): it
 * already works inside the outer sandbox.
 */
void NaClSetCreateMemoryObjectFunc(NaClCreateMemoryObjectFunc func) {
}

NaClHandle NaClCreateMemoryObject(size_t length, int executable) {
  NaClHandle memory;
  if (length % NACL_MAP_PAGESIZE) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NACL_INVALID_HANDLE;
  }
  DWORD flags;
  if (executable) {
    /*
     * Passing SEC_RESERVE overrides the implicit default of
     * SEC_COMMIT, and it means that we do not allocate swap space for
     * the pages initially.  These uncommitted pages will be
     * inaccessible even if they are mapped with PAGE_EXECUTE_READ,
     * and nacl_text.c relies on this.
     */
    flags = PAGE_EXECUTE_READWRITE | SEC_RESERVE;
  } else {
    flags = PAGE_READWRITE;
  }
  memory = CreateFileMapping(
      INVALID_HANDLE_VALUE,
      NULL,
      flags,
      (DWORD) (((unsigned __int64) length) >> 32),
      (DWORD) (length & 0xFFFFFFFF), NULL);
  return (memory == NULL) ? NACL_INVALID_HANDLE : memory;
}

/*
 * TODO(mseaborn): Reduce duplication between this function and
 * NaClHostDescMap().
 */
void* NaClMap(struct NaClDescEffector* effp,
              void* start, size_t length, int prot, int flags,
              NaClHandle memory, off_t offset) {
  static DWORD prot_to_access[] = {
    0,  /* NACL_ABI_PROT_NONE is not accepted: see below. */
    FILE_MAP_READ,
    FILE_MAP_WRITE,
    FILE_MAP_ALL_ACCESS,
    FILE_MAP_EXECUTE,
    FILE_MAP_READ | FILE_MAP_EXECUTE,
    FILE_MAP_WRITE | FILE_MAP_EXECUTE,
    FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE
  };
  DWORD desired_access;
  size_t chunk_offset;

  if (prot == NACL_ABI_PROT_NONE) {
    /*
     * There is no corresponding FILE_MAP_* option for PROT_NONE.  In
     * any case, this would not be very useful because the permissions
     * cannot later be increased beyond what was passed to
     * MapViewOfFileEx(), unlike in Unix.
     */
    NaClLog(LOG_INFO, "NaClMap: PROT_NONE not supported\n");
    SetLastError(ERROR_INVALID_PARAMETER);
    return NACL_ABI_MAP_FAILED;
  }

  if (!(flags & (NACL_ABI_MAP_SHARED | NACL_ABI_MAP_PRIVATE))) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NACL_ABI_MAP_FAILED;
  }

  /* Convert prot to the desired access type for MapViewOfFileEx(). */
  desired_access = prot_to_access[prot & 0x7];
  if (flags & NACL_ABI_MAP_PRIVATE) {
    desired_access = FILE_MAP_COPY;
  }

  CHECK((flags & NACL_ABI_MAP_FIXED) != 0);
  for (chunk_offset = 0;
       chunk_offset < length;
       chunk_offset += NACL_MAP_PAGESIZE) {
    uintptr_t chunk_addr = (uintptr_t) start + chunk_offset;
    void* mapped;

    (*effp->vtbl->UnmapMemory)(effp, chunk_addr, NACL_MAP_PAGESIZE);

    mapped = MapViewOfFileEx(memory, desired_access,
                             0, (off_t) (offset + chunk_offset),
                             NACL_MAP_PAGESIZE,
                             (void*) chunk_addr);
    if (mapped != (void*) chunk_addr) {
      NaClLog(LOG_FATAL, "nacl::Map: MapViewOfFileEx() failed, error %d\n",
              GetLastError());
    }
  }
  return start;
}
