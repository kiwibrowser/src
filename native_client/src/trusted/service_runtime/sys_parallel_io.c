/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, non-platform specific system call helper
 * routines -- for parallel I/O functions (pread/pwrite).
 *
 * NB: This is likely to be replaced with a preadv/pwritev
 * implementation, with an IRT function to build a single-element iov.
 */

#include "native_client/src/trusted/service_runtime/sys_parallel_io.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

int32_t NaClSysPRead(struct NaClAppThread *natp,
                     int32_t desc,
                     uint32_t usr_addr,
                     uint32_t buffer_bytes,
                     uint32_t offset_addr) {
  struct NaClApp *nap = natp->nap;
  struct NaClDesc *ndp = NULL;
  uintptr_t sysaddr;
  nacl_abi_off64_t offset;
  int32_t retval = -NACL_ABI_EINVAL;
  ssize_t pread_result;

  NaClLog(3,
          ("Entered NaClSysPRead(0x%08"NACL_PRIxPTR", %d, 0x%08"NACL_PRIx32
           ", %"NACL_PRIu32"[0x%"NACL_PRIx32"], 0(%"NACL_PRIx32"))\n"),
          (uintptr_t) natp, (int) desc,
          usr_addr, buffer_bytes, buffer_bytes,
          offset_addr);
  ndp = NaClAppGetDesc(nap, (int) desc);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }
  if (!NaClCopyInFromUser(nap, &offset, (uintptr_t) offset_addr,
                          sizeof offset)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  NaClLog(3,
          "... pread offset %"NACL_PRIu64" (0x%"NACL_PRIx64")\n",
          (uint64_t) offset, (uint64_t) offset);
  sysaddr = NaClUserToSysAddrRange(nap, (uintptr_t) usr_addr, buffer_bytes);
  if (kNaClBadAddress == sysaddr) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClVmIoWillStart(nap, usr_addr, usr_addr + buffer_bytes - 1);
  pread_result = (*NACL_VTBL(NaClDesc, ndp)->
                  PRead)(ndp, (void *) sysaddr, buffer_bytes, offset);
  NaClVmIoHasEnded(nap, usr_addr, usr_addr + buffer_bytes - 1);

  retval = (int32_t) pread_result;

 cleanup:
  NaClDescSafeUnref(ndp);
  return retval;
}

int32_t NaClSysPWrite(struct NaClAppThread *natp,
                      int32_t desc,
                      uint32_t usr_addr,
                      uint32_t buffer_bytes,
                      uint32_t offset_addr) {
  struct NaClApp *nap = natp->nap;
  struct NaClDesc *ndp = NULL;
  uintptr_t sysaddr;
  nacl_abi_off64_t offset;
  int32_t retval = -NACL_ABI_EINVAL;
  ssize_t pwrite_result;

  NaClLog(3,
          ("Entered NaClSysPWrite(0x%08"NACL_PRIxPTR", %d, 0x%08"NACL_PRIx32
           ", %"NACL_PRIu32"[0x%"NACL_PRIx32"], 0(%"NACL_PRIx32"))\n"),
          (uintptr_t) natp, (int) desc,
          usr_addr, buffer_bytes, buffer_bytes,
          offset_addr);
  ndp = NaClAppGetDesc(nap, (int) desc);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }
  if (!NaClCopyInFromUser(nap, &offset, (uintptr_t) offset_addr,
                          sizeof offset)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  NaClLog(3,
          "... pwrite offset %"NACL_PRIu64" (0x%"NACL_PRIx64")\n",
          (uint64_t) offset, (uint64_t) offset);
  sysaddr = NaClUserToSysAddrRange(nap, (uintptr_t) usr_addr, buffer_bytes);
  if (kNaClBadAddress == sysaddr) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClVmIoWillStart(nap, usr_addr, usr_addr + buffer_bytes - 1);
  pwrite_result = (*NACL_VTBL(NaClDesc, ndp)->
                   PWrite)(ndp, (void *) sysaddr, buffer_bytes, offset);
  NaClVmIoHasEnded(nap, usr_addr, usr_addr + buffer_bytes - 1);

  retval = (int32_t) pwrite_result;

 cleanup:
  NaClDescSafeUnref(ndp);
  return retval;
}
