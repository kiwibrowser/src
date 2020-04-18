/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* @file
 *
 * Implementation of service runtime effector subclass used for all
 * application threads.
 */
#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"

#include "native_client/src/trusted/service_runtime/nacl_desc_effector_ldr.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"


static struct NaClDescEffectorVtbl const NaClDescEffectorLdrVtbl;  /* fwd */

int NaClDescEffectorLdrCtor(struct NaClDescEffectorLdr *self,
                            struct NaClApp             *nap) {
  self->base.vtbl = &NaClDescEffectorLdrVtbl;
  self->nap = nap;
  return 1;
}

#if NACL_WINDOWS
static void NaClDescEffLdrUnmapMemory(struct NaClDescEffector  *vself,
                                      uintptr_t                sysaddr,
                                      size_t                   nbytes) {
  struct NaClDescEffectorLdr  *self = (struct NaClDescEffectorLdr *) vself;
  uintptr_t                   addr;
  uintptr_t                   endaddr;
  uintptr_t                   usraddr;
  struct NaClVmmapEntry const *map_region;

  NaClLog(4,
          ("NaClDescEffLdrUnmapMemory(0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR
           ", 0x%"NACL_PRIxS")\n"),
          (uintptr_t) vself, (uintptr_t) sysaddr, nbytes);

  for (addr = sysaddr, endaddr = sysaddr + nbytes;
       addr < endaddr;
       addr += NACL_MAP_PAGESIZE) {
    usraddr = NaClSysToUser(self->nap, addr);

    map_region = NaClVmmapFindPage(&self->nap->mem_map,
                                   usraddr >> NACL_PAGESHIFT);
    /*
     * When mapping beyond the end of file, the mapping will be rounded to
     * the 64k page boundary and the remaining space will be marked as
     * inaccessible by marking the pages as MEM_RESERVE.
     *
     * When unmapping the memory region, we use the file size, recorded in
     * the VmmapEntry to prevent a race condition when file size changes
     * after it was mmapped, together with the page num and offset to check
     * whether the page is the one backed by the file, in which case we
     * need to unmap it, or whether it's one of the tail pages backed by the
     * virtual memory in which case we need to release it.
     */
    if (NULL != map_region &&
        NULL != map_region->desc &&
        (map_region->offset + (usraddr -
            (map_region->page_num << NACL_PAGESHIFT))
         < (uintptr_t) map_region->file_size)) {
      if (!UnmapViewOfFile((void *) addr)) {
        NaClLog(LOG_FATAL,
                ("NaClDescEffLdrUnmapMemory: UnmapViewOfFile failed at"
                 " user addr 0x%08"NACL_PRIxPTR" (sys 0x%08"NACL_PRIxPTR")"
                 " error %d\n"),
                usraddr, addr, GetLastError());
      }
    } else {
      /*
       * No memory in address space, and we have only MEM_RESERVE'd
       * the address space; or memory is in address space, but not
       * backed by a file.
       */
      if (!VirtualFree((void *) addr, 0, MEM_RELEASE)) {
        NaClLog(LOG_FATAL,
                ("NaClDescEffLdrUnmapMemory: VirtualFree at user addr"
                 " 0x%08"NACL_PRIxPTR" (sys 0x%08"NACL_PRIxPTR") failed:"
                 " error %d\n"),
                usraddr, addr, GetLastError());
      }
    }
  }
}

#else  /* NACL_WINDOWS */

static void NaClDescEffLdrUnmapMemory(struct NaClDescEffector  *vself,
                                      uintptr_t                sysaddr,
                                      size_t                   nbytes) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
}
#endif  /* NACL_WINDOWS */

static struct NaClDescEffectorVtbl const NaClDescEffectorLdrVtbl = {
  NaClDescEffLdrUnmapMemory,
};
