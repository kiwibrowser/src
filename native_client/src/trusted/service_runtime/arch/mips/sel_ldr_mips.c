/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/arch/mips/sel_ldr_mips.h"
#include "native_client/src/trusted/service_runtime/arch/mips/tramp_mips.h"


/*
 * NOTE: the trampoline organization for MIPS is currenly assuming
 * NACL_TRAMPOLINE_SIZE == 32. This is contrary to the bundle size
 * which is 16.
 */

/*
 * Install a syscall trampoline at target_addr.  NB: Thread-safe.
 * The code being patched is from tramp.S
 */
void  NaClPatchOneTrampoline(struct NaClApp *nap,
                             uintptr_t      target_addr) {
  struct NaClPatchInfo  patch_info;
  uint16_t upper, lower;
  char *tramp_ptr = (char *)&NaCl_trampoline_seg_code;
  void (*funcptr)(void) = NaClSyscallSeg;
  uint32_t func_addr = (uint32_t) funcptr;
  unsigned long tramp_buffer[8];
  size_t tramp_size = ((uintptr_t) &NaCl_trampoline_seg_end
                       - (uintptr_t) &NaCl_trampoline_seg_code);

  UNREFERENCED_PARAMETER(nap);

  /*
   * We copy trampoline code to buffer so that we can patch it with address
   * of NaClSyscallSeg.
   */

  CHECK(tramp_size <= sizeof(tramp_buffer));
  memcpy(tramp_buffer, tramp_ptr, tramp_size);

  /*
   * For MIPS we do not need to patch ds, cs segments.
   */

  NaClPatchInfoCtor(&patch_info);

  /*
   * We break address of NaClSyscallSeg into upper and lower 16 bits, so that
   * we can patch first and second instruction of trampoline respectively.
   */

  upper = (uint16_t) (func_addr >> 16);
  lower = (uint16_t) (func_addr & 0xffff);

  tramp_buffer[0] = (tramp_buffer[0] & (0xFFFF0000)) | upper;
  tramp_buffer[1] = (tramp_buffer[1] & (0xFFFF0000)) | lower;

  patch_info.dst = target_addr;
  patch_info.src = (uintptr_t) tramp_buffer;
  patch_info.nbytes = ((uintptr_t) &NaCl_trampoline_seg_end
                       - (uintptr_t) &NaCl_trampoline_seg_code);

  NaClApplyPatchToMemory(&patch_info);
}

void NaClFillMemoryRegionWithHalt(void *start, size_t size) {
  uint32_t *inst = (uint32_t *) start;
  uint32_t i;

  CHECK(sizeof *inst == NACL_HALT_LEN);
  CHECK(0 == size % NACL_HALT_LEN);
  /*
   * Check that the region start is 4 bytes aligned.
   */
  CHECK(0 == (uint32_t) start % NACL_HALT_LEN);

  for (i = 0; i < (size / NACL_HALT_LEN); i++)
    inst[i] = NACL_HALT_OPCODE;
}


void NaClFillTrampolineRegion(struct NaClApp *nap) {
  NaClFillMemoryRegionWithHalt((void *)(nap->mem_start + NACL_TRAMPOLINE_START),
                               NACL_TRAMPOLINE_SIZE);
}

void  NaClLoadSpringboard(struct NaClApp  *nap) {
  UNREFERENCED_PARAMETER(nap);
}

