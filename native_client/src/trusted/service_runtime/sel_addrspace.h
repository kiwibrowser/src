/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_ADDRSPACE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_ADDRSPACE_H_ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

struct NaClApp; /* fwd */

EXTERN_C_BEGIN

#if NACL_LINUX
extern size_t g_prereserved_sandbox_size;
#endif

enum NaClAslrMode {
  NACL_DISABLE_ASLR,
  NACL_ENABLE_ASLR
};

/*
 * Try to find prereserved sandbox memory.  Sets *p to the start of the
 * sandbox.
 *
 * Returns a non-zero value if prereserved sandbox memory was found and the
 * memory is the same size as the requested size.  Returns zero otherwise.
 */
int NaClFindPrereservedSandboxMemory(void **p, size_t num_bytes);

void NaClAddrSpaceBeforeAlloc(size_t guarded_addrsp_size);

/*
 * Allocate the NaCl module's address space.  The |aslr_mode|
 * argument is passed to NaClAllocateSpaceAslr and may be ignored on
 * platforms where the sandbox requires a zero-base, e.g., ARM.
 */
NaClErrorCode NaClAllocAddrSpaceAslr(struct NaClApp *nap,
                                     enum NaClAslrMode aslr_mode) NACL_WUR;

/*
 * Old interface.  Invokes NaClAllocAddrSpaceAlsr with aslr_mode =
 * NACL_ENABLE_ASLR.
 */
NaClErrorCode NaClAllocAddrSpace(struct NaClApp *nap) NACL_WUR;

/*
 * Apply memory protection to memory regions.
 */
NaClErrorCode NaClMemoryProtection(struct NaClApp *nap) NACL_WUR;

/*
 * Platform-specific routine to allocate memory space for the NaCl
 * module.  mem is an out argument; addrsp_size is the requested
 * address space size, currently always ((size_t) 1) <<
 * nap->addr_bits.  On x86-64, there's a further requirement that this
 * is 4G.
 *
 * If |aslr_mode| is NACL_ENABLE_ASLR, this routine will attempt to
 * pick a random address for the address space.  If we are running on
 * a platform where a zero-based address space is pre-reserved (for
 * Atom performance issues), then |aslr_mode| may be ignored.
 *
 * The actual amount of memory allocated is larger than requested on
 * x86-64 and on the ARM, since guard pages are also allocated to be
 * contiguous with the allocated address space.
 *
 * If successful, the guard pages are also mapped as inaccessible (PROT_NONE).
 *
 * Returns LOAD_OK on success.
 */
NaClErrorCode NaClAllocateSpaceAslr(void **mem, size_t addrsp_size,
                                    enum NaClAslrMode aslr_mode) NACL_WUR;

/*
 * Old interface.  Invokes NaClAllocateSpaceAslr with aslr_mode =
 * NACL_ENABLE_ASLR.
 */
NaClErrorCode NaClAllocateSpace(void **mem, size_t addrsp_size) NACL_WUR;

/*
 * NaClAddrSpaceFree() unmaps all of untrusted address space.  This is
 * only safe if no untrusted threads are running.
 *
 * Note that this does not free any other data structures associated
 * with the NaClApp.  In particular, it does not free mem_map.
 */
void NaClAddrSpaceFree(struct NaClApp *nap);

EXTERN_C_END

#endif
