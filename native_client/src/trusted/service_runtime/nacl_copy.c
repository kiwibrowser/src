/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#if defined(MEMORY_SANITIZER)
#include <sanitizer/msan_interface.h>
#endif

#include "native_client/src/trusted/service_runtime/nacl_copy.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

int NaClCopyInFromUser(struct NaClApp *nap,
                       void           *dst_sys_ptr,
                       uintptr_t      src_usr_addr,
                       size_t         num_bytes) {
  uintptr_t src_sys_addr;

  src_sys_addr = NaClUserToSysAddrRange(nap, src_usr_addr, num_bytes);
  if (kNaClBadAddress == src_sys_addr) {
    return 0;
  }
  NaClCopyTakeLock(nap);
  memcpy((void *) dst_sys_ptr, (void *) src_sys_addr, num_bytes);
  NaClCopyDropLock(nap);

  return 1;
}

int NaClCopyInFromUserAndDropLock(struct NaClApp *nap,
                                  void           *dst_sys_ptr,
                                  uintptr_t      src_usr_addr,
                                  size_t         num_bytes) {
  uintptr_t src_sys_addr;

  src_sys_addr = NaClUserToSysAddrRange(nap, src_usr_addr, num_bytes);
  if (kNaClBadAddress == src_sys_addr) {
    return 0;
  }

  memcpy((void *) dst_sys_ptr, (void *) src_sys_addr, num_bytes);
  NaClCopyDropLock(nap);

  return 1;
}

int NaClCopyInFromUserZStr(struct NaClApp *nap,
                           char           *dst_buffer,
                           size_t         dst_buffer_bytes,
                           uintptr_t      src_usr_addr) {
  uintptr_t src_sys_addr;

  CHECK(dst_buffer_bytes > 0);
  src_sys_addr = NaClUserToSysAddr(nap, src_usr_addr);
  if (kNaClBadAddress == src_sys_addr) {
    dst_buffer[0] = '\0';
    return 0;
  }
  NaClCopyTakeLock(nap);
  strncpy(dst_buffer, (char *) src_sys_addr, dst_buffer_bytes);
  NaClCopyDropLock(nap);

  /* POSIX strncpy pads with NUL characters */
  if (dst_buffer[dst_buffer_bytes - 1] != '\0') {
    dst_buffer[dst_buffer_bytes - 1] = '\0';
    return 0;
  }
  return 1;
}


int NaClCopyOutToUser(struct NaClApp  *nap,
                      uintptr_t       dst_usr_addr,
                      const void      *src_sys_ptr,
                      size_t          num_bytes) {
  uintptr_t dst_sys_addr;

  dst_sys_addr = NaClUserToSysAddrRange(nap, dst_usr_addr, num_bytes);
  if (kNaClBadAddress == dst_sys_addr) {
    return 0;
  }

#if defined(MEMORY_SANITIZER)
  /*
   * Make sure we don't leak information into the sandbox by copying
   * uninitialized values.
   */
  __msan_check_mem_is_initialized(src_sys_ptr, num_bytes);
#endif

  NaClCopyTakeLock(nap);
  memcpy((void *) dst_sys_addr, src_sys_ptr, num_bytes);
  NaClCopyDropLock(nap);

  return 1;
}
