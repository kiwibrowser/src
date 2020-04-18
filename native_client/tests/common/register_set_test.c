/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <setjmp.h>

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/tests/common/register_set.h"


/*
 * This tests whether REGS_SAVER_FUNC calls
 * CheckStackAlignmentMiddle() with the stack pointer aligned.  In
 * order to test this we use REGS_SAVER_FUNC itself!
 */
REGS_SAVER_FUNC(CheckStackAlignmentEntry, CheckStackAlignmentMiddle);
REGS_SAVER_FUNC_NOPROTO(CheckStackAlignmentMiddle, CheckStackAlignment);

struct NaClSignalContext g_regs;
jmp_buf g_jmp_buf;

void CheckStackAlignment(struct NaClSignalContext *regs) {
  assert(((regs->stack_ptr + NACL_STACK_PAD_BELOW_ALIGN) &
          NACL_STACK_ALIGN_MASK) == 0);
  longjmp(g_jmp_buf, 1);
}

void test_stack_alignment(void) {
  char stack[0x1000];
  int offset;
  for (offset = 0; offset < 64; offset++) {
    RegsFillTestValues(&g_regs, /* seed= */ 0);
    g_regs.stack_ptr = (uintptr_t) stack + sizeof(stack) - offset;
    RegsApplySandboxConstraints(&g_regs);
    if (!setjmp(g_jmp_buf)) {
      JUMP_WITH_REGS(&g_regs, CheckStackAlignmentEntry);
    }
  }
}

int main(void) {
  test_stack_alignment();
  return 0;
}
