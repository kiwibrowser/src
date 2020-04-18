/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_random.h"

#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


/*
 * This syscall copies freshly-generated random data into the supplied
 * buffer.
 *
 * Ideally this operation would be provided via a NaClDesc rather than via
 * a dedicated syscall.  However, if random data is read using the read()
 * syscall, it is too easy for an innocent-but-buggy application to
 * accidentally close() a random-data-source file descriptor, and then
 * read() from a file that is subsequently opened with the same FD number.
 * If the application relies on random data being unguessable, this could
 * make the application vulnerable (e.g. see https://crbug.com/374383).
 * This could be addressed by having a NaClDesc operation that can't
 * accidentally be confused with a read(), but that would be more
 * complicated.
 *
 * Providing a dedicated syscall is simple and removes that risk.
 */
int32_t NaClSysGetRandomBytes(struct NaClAppThread *natp,
                              uint32_t buf_addr, uint32_t buf_size) {
  struct NaClApp *nap = natp->nap;

  uintptr_t sysaddr = NaClUserToSysAddrRange(nap, buf_addr, buf_size);
  if (sysaddr == kNaClBadAddress)
    return -NACL_ABI_EFAULT;

  /*
   * Since we don't use NaClCopyOutToUser() for writing the data into the
   * sandbox, we use NaClVmIoWillStart()/NaClVmIoHasEnded() to ensure that
   * no mmap hole is opened up while we write the data.
   */
  NaClVmIoWillStart(nap, buf_addr, buf_addr + buf_size - 1);
  NaClGlobalSecureRngGenerateBytes((uint8_t *) sysaddr, buf_size);
  NaClVmIoHasEnded(nap, buf_addr, buf_addr + buf_size - 1);

  return 0;
}
