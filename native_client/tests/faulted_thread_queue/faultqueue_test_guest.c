/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/tests/common/register_set.h"


/*
 * This test program sets registers to a known state and then faults,
 * so that faultqueue_test_host.c can check that it receives
 * notification of the fault.
 *
 * faultqueue_test_host.c makes execution resume from the instruction
 * after FaultAddr.  On x86, it also checks that single-stepping these
 * following instructions works.
 */

void FaultAddr(void);

jmp_buf return_jmp_buf;

void DoLongjmp(void) {
  longjmp(return_jmp_buf, 1);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Expected 1 argument: <memory-address>\n");
    return 1;
  }
  char *end;
  struct NaClSignalContext *expected_regs =
      (struct NaClSignalContext *) strtoul(argv[1], &end, 0);
  assert(*end == '\0');

  char stack[0x10000];

  RegsFillTestValues(expected_regs, /* seed= */ 0);
  expected_regs->stack_ptr = (uintptr_t) stack + sizeof(stack);
  expected_regs->prog_ctr = (uintptr_t) FaultAddr;
  RegsApplySandboxConstraints(expected_regs);

  if (!setjmp(return_jmp_buf)) {
#if defined(__i386__) || defined(__x86_64__)
    ASM_WITH_REGS(
        expected_regs,
        /*
         * Align so that the following instructions do not cross an
         * instruction bundle boundary.
         */
        ".p2align 5\n"
        "FaultAddr: hlt\n"
        /* Test single-stepping with some no-op instructions of known sizes. */
        ".byte 0x90\n" /* nop */
        ".byte 0x66, 0x90\n" /* xchg %ax, %ax */
        ".byte 0x0f, 0x1f, 0x00\n" /* nopl (%eax) */
        ".byte 0x0f, 0x1f, 0x40, 0x00\n" /* nopl 0x0(%eax) */
        ".byte 0x0f, 0x1f, 0x44, 0x00, 0x00\n" /* nopl 0x0(%eax, %eax, 1) */
        "jmp DoLongjmp\n");
#elif defined(__arm__)
    ASM_WITH_REGS(
        expected_regs,
        ".p2align 4\n"
        "FaultAddr: .word " NACL_TO_STRING(NACL_INSTR_ARM_ABORT_NOW) "\n"
        /*
         * ARM does not provide hardware single-stepping so we do not
         * test it here, unlike in the x86 case.
         */
        "b DoLongjmp\n"
        ".p2align 4\n");
#elif defined(__mips__)
    ASM_WITH_REGS(
        expected_regs,
        ".p2align 4\n"
        ".global FaultAddr\n"
        "FaultAddr: .word " NACL_TO_STRING(NACL_HALT_WORD) "\n"
        "nop\n"
        /*
         * MIPS does not provide hardware single-stepping so we do not
         * test it here, unlike in the x86 case.
         */
        "lui $t9, %%hi(DoLongjmp)\n"
        "addiu $t9, $t9, %%lo(DoLongjmp)\n"
        "and $t9, $t9, $t6\n"
        "jr $t9\n"
        "nop\n"
        ".p2align 4\n");
#else
# error Unknown architecture
#endif
  }

  /*
   * Avoid calling exit().  This nexe's _start() entry point is called
   * multiple times by faultqueue_test_host.c, without resetting the
   * data segment, which is unusual.  This causes exit() to hang when
   * libpthread is linked in, which recent PNaCl toolchains have
   * started to do by default.
   */
  _exit(0);
}
