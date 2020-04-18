/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <windows.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"

#if NACL_BUILD_SUBARCH == 32

/*
 * This function searches for sandbox memory that has been reserved by
 * the parent process on our behalf. We prereserve the sandbox on 32-bit
 * systems because otherwise the address space may become fragmented, making
 * the large sandbox request fail.
 */
int NaClFindPrereservedSandboxMemory(void **p, size_t num_bytes) {
  SYSTEM_INFO sys_info;
  MEMORY_BASIC_INFORMATION mem;
  char *start;
  SIZE_T mem_size;

  GetSystemInfo(&sys_info);
  start = sys_info.lpMinimumApplicationAddress;
  while (1) {
    mem_size = VirtualQuery((LPCVOID)start, &mem, sizeof(mem));
    if (mem_size == 0)
      break;
    CHECK(mem_size == sizeof(mem));

    if (mem.State == MEM_RESERVE &&
        mem.AllocationProtect == PAGE_NOACCESS &&
        mem.RegionSize == num_bytes) {
      if (!VirtualFree(start, 0, MEM_RELEASE)) {
        DWORD err = GetLastError();
        NaClLog(LOG_FATAL,
                "NaClFindPrereservedSandboxMemory: VirtualFree(0x%016"
                NACL_PRIxPTR", 0, MEM_RELEASE) failed "
                "with error 0x%X\n",
                (uintptr_t) start, err);
      }
      *p = start;
      return 0;
    }
    start += mem.RegionSize;
    if ((LPVOID)start >= sys_info.lpMaximumApplicationAddress)
      break;
  }
  return -ENOMEM;
}

#endif  /* NACL_BUILD_SUBARCH == 32 */

/*
 * Nothing to do here on Windows.
 */
void NaClAddrSpaceBeforeAlloc(size_t untrusted_size) {
  UNREFERENCED_PARAMETER(untrusted_size);
}
