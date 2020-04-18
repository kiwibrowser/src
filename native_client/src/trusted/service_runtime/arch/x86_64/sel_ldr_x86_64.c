/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/portability_string.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/springboard.h"
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"
#include "native_client/src/trusted/service_runtime/arch/x86_64/sel_rt_64.h"
#include "native_client/src/trusted/service_runtime/arch/x86_64/tramp_64.h"

static uintptr_t AddDispatchAddr(uintptr_t *next_addr,
                                 uintptr_t target_routine) {
  uintptr_t addr = *next_addr;
  *(uintptr_t *) addr = target_routine;
  *next_addr += sizeof(uintptr_t);
  return addr;
}

int NaClMakeDispatchAddrs(struct NaClApp *nap) {
  int                   retval = 0;  /* fail */
  int                   error;
  void                  *page_addr = NULL;
  uintptr_t             next_addr;
  uintptr_t             nacl_syscall_addr = 0;
  uintptr_t             get_tls_fast_path1_addr = 0;
  uintptr_t             get_tls_fast_path2_addr = 0;

  NaClLog(2, "Entered NaClMakeDispatchAddrs\n");
  if (0 != nap->nacl_syscall_addr) {
    NaClLog(LOG_ERROR, " dispatch addrs already initialized!\n");
    return 1;
  }

  if (0 != (error = NaClPageAllocRandomized(&page_addr,
                                            NACL_MAP_PAGESIZE))) {
    NaClLog(LOG_INFO,
            "NaClMakeDispatchAddrs::NaClPageAlloc failed, errno %d\n",
            -error);
    retval = 0;
    goto cleanup;
  }
  NaClLog(2, "NaClMakeDispatchAddrs: got addr 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) page_addr);

  if (0 != (error = NaClMprotect(page_addr,
                                 NACL_MAP_PAGESIZE,
                                 PROT_READ | PROT_WRITE))) {
    NaClLog(LOG_INFO,
            "NaClMakeDispatchAddrs::NaClMprotect r/w failed, errno %d\n",
            -error);
    retval = 0;
    goto cleanup;
  }

  next_addr = (uintptr_t) page_addr;
  nacl_syscall_addr =
      AddDispatchAddr(&next_addr, (uintptr_t) &NaClSyscallSeg);
  get_tls_fast_path1_addr =
      AddDispatchAddr(&next_addr, (uintptr_t) &NaClGetTlsFastPath1);
  get_tls_fast_path2_addr =
      AddDispatchAddr(&next_addr, (uintptr_t) &NaClGetTlsFastPath2);

  if (0 != (error = NaClMprotect(page_addr, NACL_MAP_PAGESIZE, PROT_READ))) {
    NaClLog(LOG_INFO,
            "NaClMakeDispatchAddrs::NaClMprotect read-only failed, errno %d\n",
            -error);
    retval = 0;
    goto cleanup;
  }
  retval = 1;
 cleanup:
  if (0 == retval) {
    if (NULL != page_addr) {
      NaClPageFree(page_addr, NACL_MAP_PAGESIZE);
      page_addr = NULL;
    }
  } else {
    nap->nacl_syscall_addr = nacl_syscall_addr;
    nap->get_tls_fast_path1_addr = get_tls_fast_path1_addr;
    nap->get_tls_fast_path2_addr = get_tls_fast_path2_addr;
  }
  return retval;
}

/*
 * Install a syscall trampoline at target_addr.  NB: Thread-safe.
 */
void  NaClPatchOneTrampolineCall(uintptr_t  call_target_addr,
                                 uintptr_t  target_addr) {
  struct NaClPatchInfo  patch_info;
  struct NaClPatch      tramp_addr;
  struct NaClPatch      call_target;

  tramp_addr.target = (((uintptr_t) &NaCl_trampoline_tramp_addr)
                       - sizeof(uint32_t));
  tramp_addr.value = (uint32_t) target_addr;

  NaClLog(6, "call_target_addr = 0x%"NACL_PRIxPTR"\n", call_target_addr);
  CHECK(0 != call_target_addr);
  call_target.target = (((uintptr_t) &NaCl_trampoline_call_target)
                        - sizeof(uintptr_t));
  call_target.value = call_target_addr;

  NaClPatchInfoCtor(&patch_info);

  patch_info.abs64 = &call_target;
  patch_info.num_abs64 = 1;

  patch_info.abs32 = &tramp_addr;
  patch_info.num_abs32 = 1;

  patch_info.dst = target_addr;
  patch_info.src = (uintptr_t) &NaCl_trampoline_code;
  patch_info.nbytes = ((uintptr_t) &NaCl_trampoline_code_end
                       - (uintptr_t) &NaCl_trampoline_code);
  CHECK(patch_info.nbytes <= NACL_INSTR_BLOCK_SIZE);

  NaClApplyPatchToMemory(&patch_info);
}

void NaClPatchOneTrampoline(struct NaClApp *nap, uintptr_t target_addr) {
  NaClPatchOneTrampolineCall(nap->nacl_syscall_addr, target_addr);
}

void NaClFillMemoryRegionWithHalt(void *start, size_t size) {
  CHECK(!(size % NACL_HALT_LEN));
  /* Tell valgrind that this memory is accessible and undefined */
  NACL_MAKE_MEM_UNDEFINED(start, size);
  memset(start, NACL_HALT_OPCODE, size);
}

void NaClFillTrampolineRegion(struct NaClApp *nap) {
  NaClFillMemoryRegionWithHalt(
      (void *) (nap->mem_start + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE);
}

void NaClLoadSpringboard(struct NaClApp  *nap) {
  /*
   * There is no springboard for x86-64.
   */
  UNREFERENCED_PARAMETER(nap);
}
