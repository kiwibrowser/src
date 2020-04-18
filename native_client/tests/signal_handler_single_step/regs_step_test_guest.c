/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"
#include "native_client/tests/common/register_set.h"
#include "native_client/tests/signal_handler_single_step/step_test_common.h"


/*
 * This test program calls a NaCl syscall in an infinite loop with
 * callee-saved registers set to known values.  These register values
 * are saved to a memory address for cross-checking by the
 * trusted-code portion of the test case.
 */

jmp_buf g_jmp_buf;
uint32_t g_regs_should_match;

#if defined(__i386__)
# define SYSCALL_CALLER(suffix) \
    __asm__(".pushsection .text, \"ax\", @progbits\n" \
            "SyscallCaller" suffix ":\n" \
            "movl $1, g_regs_should_match\n" \
            "naclcall %esi\n" \
            "SyscallReturnAddress" suffix ":\n" \
            "movl $0, g_regs_should_match\n" \
            "jmp ReturnFromSyscall\n" \
            ".popsection\n");
#elif defined(__x86_64__)
# define SYSCALL_CALLER(suffix) \
    __asm__(".pushsection .text, \"ax\", @progbits\n" \
            "SyscallCaller" suffix ":\n" \
            "movl $1, g_regs_should_match(%rip)\n" \
            /* Call via a temporary register so as not to modify %r12. */ \
            "movl %r12d, %eax\n" \
            "naclcall %eax, %r15\n" \
            "SyscallReturnAddress" suffix ":\n" \
            "movl $0, g_regs_should_match(%rip)\n" \
            "jmp ReturnFromSyscall\n" \
            ".popsection\n");
#elif defined(__arm__)
# define SYSCALL_CALLER(suffix) \
    __asm__(".pushsection .text, \"ax\", %progbits\n" \
            ".p2align 4\n" \
            "SyscallCaller" suffix ":\n" \
            /* Set g_regs_should_match = 1. */ \
            "bic r5, r5, #0xc0000000\n" \
            "str r6, [r5]\n" \
            /* Call syscall. */ \
            "adr lr, SyscallReturnAddress" suffix "\n" \
            "nop\n" /* Pad to bundle-align next instruction */ \
            "bic r4, r4, #0xc000000f\n" \
            "bx r4\n" \
            ".p2align 4\n" \
            "SyscallReturnAddress" suffix ":\n" \
            /* Set g_regs_should_match = 0. */ \
            "bic r5, r5, #0xc0000000\n" \
            "str r7, [r5]\n" \
            "b ReturnFromSyscall\n" \
            ".popsection\n");
#elif defined(__mips__)
# define SYSCALL_CALLER(suffix) \
    __asm__(".pushsection .text, \"ax\", @progbits\n" \
            ".p2align 4\n" \
            ".set noreorder\n" \
            ".globl SyscallCaller" suffix "\n" \
            "SyscallCaller" suffix ":\n" \
            /* Set g_regs_should_match = 1. */ \
            "sw $s2, 0($s1)\n" \
            /* Call syscall. */ \
            "jalr $s0\n" \
            "nop\n" \
            ".p2align 4\n" \
            ".globl SyscallReturnAddress" suffix "\n" \
            "SyscallReturnAddress" suffix ":\n" \
            /* Set g_regs_should_match = 0. */ \
            "sw $zero, 0($s1)\n" \
            "lui $t9, %hi(ReturnFromSyscall)\n" \
            "b ReturnFromSyscall\n" \
            "addiu $t9, $t9, %lo(ReturnFromSyscall)\n" \
            ".set reorder\n" \
            ".popsection\n");
#else
# error Unsupported architecture
#endif

SYSCALL_CALLER("1")
SYSCALL_CALLER("2")
void SyscallCaller1(void);
void SyscallCaller2(void);
void SyscallReturnAddress1(void);
void SyscallReturnAddress2(void);

void ReturnFromSyscall(void) {
  longjmp(g_jmp_buf, 1);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Expected 1 argument: <memory-address>\n");
    return 1;
  }

  char *end;
  struct RegsTestShm *test_shm =
      (struct RegsTestShm *) strtoul(argv[1], &end, 0);
  assert(*end == '\0');
  test_shm->regs_should_match = &g_regs_should_match;

  struct NaClSignalContext call_regs;
  char stack[0x10000];

  int call_count = 0;
  for (call_count = 0; ; call_count++) {
    uintptr_t syscall_addr;
    /*
     * Test fast-path TLS syscalls.  We shoe-horn these in after the
     * first call to test_syscall_1 has enabled single-stepping.
     */
    if (call_count == 1) {
      syscall_addr = NACL_SYSCALL_ADDR(NACL_sys_tls_get);
    } else if (call_count == 2) {
      syscall_addr = NACL_SYSCALL_ADDR(NACL_sys_second_tls_get);
    } else {
      syscall_addr = NACL_SYSCALL_ADDR(NACL_sys_test_syscall_1);
    }

    /*
     * Use different expected register values for each call.
     * Otherwise, the test could accidentally pass because the
     * stack_ptr reported during the entry to a syscall can happen to
     * match the stack_ptr saved by the previous syscall.
     */
    RegsFillTestValues(&call_regs, /* seed= */ call_count);
#if defined(__i386__)
    call_regs.esi = syscall_addr;
#elif defined(__x86_64__)
    call_regs.r12 = syscall_addr;
#elif defined(__arm__)
    call_regs.r4 = syscall_addr;
    call_regs.r5 = (uintptr_t) &g_regs_should_match;
    call_regs.r6 = 1;
    call_regs.r7 = 0;
#elif defined(__mips__)
    call_regs.s0 = syscall_addr;
    call_regs.s1 = (uintptr_t) &g_regs_should_match;
    call_regs.s2 = 1;
#else
# error Unsupported architecture
#endif
    call_regs.prog_ctr = (uintptr_t) (call_count % 2 == 0
                                      ? SyscallReturnAddress1
                                      : SyscallReturnAddress2);
    call_regs.stack_ptr =
        (uintptr_t) stack + sizeof(stack) - (call_count % 2) * 0x100;
    RegsApplySandboxConstraints(&call_regs);
    RegsUnsetNonCalleeSavedRegisters(&call_regs);
    test_shm->expected_regs = call_regs;
    if (!setjmp(g_jmp_buf)) {
      if (call_count % 2 == 0) {
        JUMP_WITH_REGS(&call_regs, SyscallCaller1);
      } else {
        JUMP_WITH_REGS(&call_regs, SyscallCaller2);
      }
    }
  }
}
