/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/arch/arm/sel_ldr_arm.h"
#include "native_client/src/trusted/service_runtime/arch/arm/tramp_arm.h"


/* NOTE(robertm): the trampoline organization for ARM is currenly assuming
 * NACL_TRAMPOLINE_SIZE == 32. This is contrary to the bundle size
 * which is 16. The reason for this is tramp.S which has a payload
 * 5 instr + one data item
 */

void  NaClPatchOneTrampoline(struct NaClApp *nap,
                             uintptr_t      target_addr) {
  size_t trampoline_size = ((uintptr_t) &NaCl_trampoline_seg_end
                            - (uintptr_t) &NaCl_trampoline_seg_code);

  UNREFERENCED_PARAMETER(nap);

  CHECK(trampoline_size == NACL_SYSCALL_BLOCK_SIZE);
  memcpy((void *) target_addr, &NaCl_trampoline_seg_code,
         NACL_SYSCALL_BLOCK_SIZE);
}


void NaClFillMemoryRegionWithHalt(void *start, size_t size) {
  uint32_t *inst = (uint32_t *) start;
  uint32_t i;

  CHECK(sizeof *inst == NACL_HALT_LEN);
  CHECK(0 == size % NACL_HALT_LEN);
  /* check that the region start is 4 bytes aligned */
  CHECK(0 == (uint32_t)start % NACL_HALT_LEN);

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
