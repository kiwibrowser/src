/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_private.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_sysbrk(void **newbrk) {
  /*
   * The syscall does not actually indicate error.  It just returns the
   * new current value, which is unchanged if something went wrong.
   * But if the requested value was below the end of the data segment,
   * the new value will be greater, but this is not "going wrong".
   * Here we just approximate a saner interface: you get what you requested,
   * you did a "probe" request passing NULL in, or it's an error.
   * TODO(mcgrathr): this interface should just go away!!
   */
  void *requested = *newbrk;
  void *got = NACL_SYSCALL(brk)(requested);

  if (got == requested || requested == NULL) {
    *newbrk = got;
    return 0;
  }

  return ENOMEM;
}

static int nacl_irt_mmap(void **addr, size_t len,
                         int prot, int flags, int fd, off_t off) {
  /*
   * We currently do not allow PROT_EXEC to be called without MAP_FIXED, but
   * potentially we could call allocate_code_data at this point and turn it
   * into a MAP_FIXED call. When we do this we probably should be able to hold
   * the code/data allocation mutex to minimize fragmentation.
   */
  uint32_t rv = (uintptr_t) NACL_SYSCALL(mmap)(*addr, len, prot, flags,
                                               fd, &off);
  if ((uint32_t) rv > 0xffff0000u)
    return -(int32_t) rv;
  *addr = (void *) (uintptr_t) rv;

  /*
   * When PROT_EXEC flag is set, we must coordinate code segments with
   * code/data allocations so they are interlocked. This is safe to do at the
   * end because of the constraint that code allocations must be mapped with
   * MAP_FIXED. MAP_FIXED is specified to discard any already mapped pages if an
   * already mapped page overlaps with the set of pages being mapped. This means
   * if 2 threads happen to overlap a set of pages through both mmap and
   * nacl_irt_code_data_allocate simultenously, it does not introduce any extra
   * race conditions as having mapped them both sequentially. The code blocks
   * must both already be coordinating through the code/data allocation
   * functions, or through some other external means.
   *
   * For example, if mmap and nacl_irt_code_data_allocate were both called
   * simultaneously, 3 conditions could occur:
   *   1. If one of the calls fails, the failed function will not have mapped
   *      anything so the other one will simply succeed. Note this section
   *      which calls irt_reserve_code_allocation is only done on the success
   *      case.
   *   2. If both succeed, but nacl_irt_code_data_allocate allocates the code
   *      address before the NACL_SYSCALL(mmap) call or after
   *      the reserve code allocation call, it is not really interwoven
   *      this case is equivalent to simple sequential calls.
   *   3. If both succeed, but nacl_irt_code_data_allocate allocates the code
   *      address after the NACL_SYSCALL(mmap) but before the reserve code
   *      allocation call, the reserve code allocation call could be too late
   *      and attempt to reserve what the code/data allocation happened to
   *      return. But because the mmap call was mapped with MAP_FIXED, it would
   *      not have made a difference. It is as if someone called
   *      nacl_irt_code_data_allocate first, then called the mmap with MAP_FIXED
   *      at the same location.
   */
  if (prot & PROT_EXEC) {
    irt_reserve_code_allocation((uintptr_t) *addr, len);
  }
  return 0;
}

/*
 * mmap from nacl-irt-memory-0.1 interface should ignore PROT_EXEC bit for
 * backward-compatibility reasons.
 */
static int nacl_irt_mmap_v0_1(void **addr, size_t len,
                              int prot, int flags, int fd, off_t off) {
  return nacl_irt_mmap(addr, len, prot & ~PROT_EXEC, flags, fd, off);
}

static int nacl_irt_munmap(void *addr, size_t len) {
  return -NACL_SYSCALL(munmap)(addr, len);
}

static int nacl_irt_mprotect(void *addr, size_t len, int prot) {
  return -NACL_SYSCALL(mprotect)(addr, len, prot);
}

const struct nacl_irt_memory_v0_1 nacl_irt_memory_v0_1 = {
  nacl_irt_sysbrk,
  nacl_irt_mmap_v0_1,
  nacl_irt_munmap,
};

const struct nacl_irt_memory_v0_2 nacl_irt_memory_v0_2 = {
  nacl_irt_sysbrk,
  nacl_irt_mmap,
  nacl_irt_munmap,
  nacl_irt_mprotect,
};

const struct nacl_irt_memory nacl_irt_memory = {
  nacl_irt_mmap,
  nacl_irt_munmap,
  nacl_irt_mprotect,
};
