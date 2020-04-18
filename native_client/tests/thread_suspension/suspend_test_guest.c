/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/thread_suspension/suspend_test.h"

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"
#include "native_client/tests/common/register_set.h"


typedef int (*TYPE_nacl_test_syscall_1)(struct SuspendTestShm *test_shm);

__thread jmp_buf return_jmp_buf;
__thread struct SuspendTestShm *thread_test_shm;


static void MutatorThread(struct SuspendTestShm *test_shm) {
  uint32_t next_val = 0;
  while (!test_shm->should_exit) {
    test_shm->var = next_val++;
  }
}

static void SyscallReturnThread(struct SuspendTestShm *test_shm) {
  int rc = NACL_SYSCALL(test_syscall_1)(test_shm);
  assert(rc == 0);
  /* Set value to indicate that the syscall returned. */
  test_shm->var = 99;
}

static void SyscallInvokerThread(struct SuspendTestShm *test_shm) {
  uint32_t next_val = 0;
  while (!test_shm->should_exit) {
    NACL_SYSCALL(null)();
    test_shm->var = next_val++;
  }
}

void spin_instruction(void);

REGS_SAVER_FUNC(ContinueAfterSuspension, CheckSavedRegisters);

void CheckSavedRegisters(struct NaClSignalContext *regs) {
  RegsAssertEqual(regs, &thread_test_shm->expected_regs);
  _exit(0);
}

static void RegisterSetterThread(struct SuspendTestShm *test_shm) {
  struct NaClSignalContext *regs = &test_shm->expected_regs;
  char stack[0x10000];

  RegsFillTestValues(regs, /* seed= */ 0);
  regs->stack_ptr = (uintptr_t) stack + sizeof(stack);
  regs->prog_ctr = (uintptr_t) spin_instruction;
  RegsApplySandboxConstraints(regs);
  thread_test_shm = test_shm;

  /*
   * Set registers to known test values and then spin.  We do not
   * block by entering a NaCl syscall because that would disturb the
   * register state.
   */
  test_shm->continue_after_suspension_func =
      (uintptr_t) ContinueAfterSuspension;
  assert(offsetof(struct SuspendTestShm, var) == 0);
#if defined(__i386__)
  regs->eax = (uintptr_t) test_shm;
  ASM_WITH_REGS(
      regs,
      /* Align to ensure no NOPs are inserted in the code that follows. */
      ".p2align 5\n"
      /* Set "test_shm->var = test_shm" to indicate that we are ready. */
      "movl %%eax, (%%eax)\n"
      "spin_instruction:\n"
      "jmp spin_instruction\n");
#elif defined(__x86_64__)
  regs->rax = (uintptr_t) test_shm;
  ASM_WITH_REGS(
      regs,
      /* Align to ensure no NOPs are inserted in the code that follows. */
      ".p2align 5\n"
      /* Set "test_shm->var = test_shm" to indicate that we are ready. */
      "movl %%eax, %%nacl:(%%r15, %%rax)\n"
      "spin_instruction:\n"
      "jmp spin_instruction\n");
#elif defined(__arm__)
  regs->r0 = (uintptr_t) test_shm;
  ASM_WITH_REGS(
      regs,
      /* Align to ensure no NOPs are inserted in the code that follows. */
      ".p2align 4\n"
      /* Set "test_shm->var = test_shm" to indicate that we are ready. */
      "bic r0, r0, #0xc0000000\n"
      "str r0, [r0]\n"
      "spin_instruction:\n"
      "b spin_instruction\n");
#elif defined(__mips__)
  regs->a0 = (uintptr_t) test_shm;
  ASM_WITH_REGS(
      regs,
      /* Align to ensure no NOPs are inserted in the code that follows. */
      ".p2align 4\n"
      /* Set "test_shm->var = test_shm" to indicate that we are ready. */
      "and $a0, $a0, $t7\n"
      "sw $a0, 0($a0)\n"
      ".global spin_instruction\n"
      "spin_instruction:\n"
      "b spin_instruction\n"
      "nop\n");
#else
# error Unsupported architecture
#endif
  assert(!"Should not reach here");
}

static void ContinueAfterSyscall(void) {
  longjmp(return_jmp_buf, 1);
}

/* Set registers to known values and enter a NaCl syscall. */
static void SyscallRegisterSetterThread(struct SuspendTestShm *test_shm) {
  struct NaClSignalContext call_regs;
  char stack[0x10000];

  RegsFillTestValues(&call_regs, /* seed= */ 0);
  call_regs.stack_ptr = (uintptr_t) stack + sizeof(stack);
  call_regs.prog_ctr = (uintptr_t) ContinueAfterSyscall;
  RegsApplySandboxConstraints(&call_regs);

  /*
   * call_regs are the registers we set on entry to the syscall.
   * expected_regs are the registers that should be reported by
   * NaClAppThreadGetSuspendedRegisters().  Since not all registers
   * are saved when entering a syscall, expected_regs will be the same
   * as call_regs but with various registers zeroed out.
   */
  test_shm->expected_regs = call_regs;
  RegsUnsetNonCalleeSavedRegisters(&test_shm->expected_regs);

  uintptr_t syscall_addr = (uintptr_t) NACL_SYSCALL(test_syscall_1);
  if (!setjmp(return_jmp_buf)) {
#if defined(__i386__)
    test_shm->expected_regs.stack_ptr -= 4;  /* Account for argument */
    call_regs.eax = syscall_addr;
    call_regs.ecx = (uintptr_t) test_shm;  /* Scratch register */
    ASM_WITH_REGS(
        &call_regs,
        "push %%ecx\n"  /* Push syscall argument */
        "push $ContinueAfterSyscall\n"  /* Push return address */
        "nacljmp %%eax\n");
#elif defined(__x86_64__)
    call_regs.rax = syscall_addr;
    call_regs.rdi = (uintptr_t) test_shm;  /* Set syscall argument */
    ASM_WITH_REGS(
        &call_regs,
        "push $ContinueAfterSyscall\n"  /* Push return address */
        "nacljmp %%eax, %%r15\n");
#elif defined(__arm__)
    call_regs.r0 = (uintptr_t) test_shm;  /* Set syscall argument */
    call_regs.r1 = syscall_addr;  /* Scratch register */
    call_regs.lr = (uintptr_t) ContinueAfterSyscall;
    ASM_WITH_REGS(
        &call_regs,
        "bic r1, r1, #0xf000000f\n"
        "bx r1\n");
#elif defined(__mips__)
    call_regs.a0 = (uintptr_t) test_shm;  /* Set syscall argument */
    call_regs.t9 = syscall_addr;  /* Scratch register */
    call_regs.return_addr = (uintptr_t) ContinueAfterSyscall;
    ASM_WITH_REGS(
        &call_regs,
        "and $t9, $t9, $t6\n"
        "jr $t9\n"
        "nop\n");
#else
# error Unsupported architecture
#endif
    assert(!"Should not reach here");
  }
}

void SyscallReturnAddress(void);
#if defined(__i386__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        "SyscallLoop:"
        "naclcall %esi\n"
        "SyscallReturnAddress:\n"
        "jmp SyscallLoop\n"
        ".popsection\n");
#elif defined(__x86_64__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        "SyscallLoop:\n"
        /* Call via a temporary register so as not to modify %r12. */
        "mov %r12d, %eax\n"
        "naclcall %eax, %r15\n"
        "SyscallReturnAddress:\n"
        "jmp SyscallLoop\n"
        ".popsection\n");
#elif defined(__arm__)
__asm__(".pushsection .text, \"ax\", %progbits\n"
        ".p2align 4\n"
        "SyscallReturnAddress:\n"
        "adr lr, SyscallReturnAddress\n"
        "bic r4, r4, #0xc000000f\n"
        "bx r4\n"
        ".popsection\n");
#elif defined(__mips__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".p2align 4\n"
        ".global SyscallReturnAddress\n"
        "SyscallReturnAddress:\n"
        "lui   $ra, %hi(SyscallReturnAddress)\n"
        "addiu $ra, $ra, %lo(SyscallReturnAddress)\n"
        "and $s0, $s0, $t6\n"
        "jr $s0\n"
        "nop\n"
        ".popsection\n");
#else
# error Unsupported architecture
#endif

/*
 * Set registers to known values and call a NaCl syscall in an
 * infinite loop.  This is used for testing that the same register
 * state is reported while the thread is in untrusted code or inside
 * the syscall.
 */
static void SyscallRegisterSetterLoopThread(struct SuspendTestShm *test_shm) {
  struct NaClSignalContext *regs = &test_shm->expected_regs;
  char stack[0x10000];

  RegsFillTestValues(regs, /* seed= */ 0);
  regs->stack_ptr = (uintptr_t) stack + sizeof(stack);
  regs->prog_ctr = (uintptr_t) SyscallReturnAddress;
  RegsApplySandboxConstraints(regs);
  RegsUnsetNonCalleeSavedRegisters(regs);

  uintptr_t syscall_addr = NACL_SYSCALL_ADDR(NACL_sys_test_syscall_2);
#if defined(__i386__)
  regs->esi = syscall_addr;
#elif defined(__x86_64__)
  regs->r12 = syscall_addr;
#elif defined(__arm__)
  regs->r4 = syscall_addr;
#elif defined(__mips__)
  regs->s0 = syscall_addr;
#else
# error Unsupported architecture
#endif
  JUMP_WITH_REGS(regs, SyscallReturnAddress);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Expected 2 arguments: <test-type> <memory-address>\n");
    return 1;
  }
  char *test_type = argv[1];

  char *end;
  struct SuspendTestShm *test_shm =
      (struct SuspendTestShm *) strtoul(argv[2], &end, 0);
  assert(*end == '\0');

  if (strcmp(test_type, "MutatorThread") == 0) {
    MutatorThread(test_shm);
  } else if (strcmp(test_type, "SyscallReturnThread") == 0) {
    SyscallReturnThread(test_shm);
  } else if (strcmp(test_type, "SyscallInvokerThread") == 0) {
    SyscallInvokerThread(test_shm);
  } else if (strcmp(test_type, "RegisterSetterThread") == 0) {
    RegisterSetterThread(test_shm);
  } else if (strcmp(test_type, "SyscallRegisterSetterThread") == 0) {
    SyscallRegisterSetterThread(test_shm);
  } else if (strcmp(test_type, "SyscallRegisterSetterLoopThread") == 0) {
    SyscallRegisterSetterLoopThread(test_shm);
  } else {
    fprintf(stderr, "Unknown test type: %s\n", test_type);
    return 1;
  }
  return 0;
}
