/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <sched.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"
#include "native_client/src/untrusted/valgrind/dynamic_annotations.h"
#include "native_client/tests/common/register_set.h"


struct NaClSignalContext g_expected_regs;
jmp_buf g_return_jmp_buf;

char g_stack[0x1000];
/* We use 'volatile' because we spin waiting for this variable to be set. */
volatile int32_t g_stack_in_use;
volatile struct NaClSignalContext g_initial_thread_regs;


/*
 * Modify expected_regs to reflect how the current implementations of
 * NaClSwitch() leave the register state on entry to untrusted code.
 */
void SetNaClSwitchExpectations(struct NaClSignalContext *expected_regs) {
#if defined(__i386__) || defined(__x86_64__)
  /*
   * The context switch contains an "xorl %rN, %rN" to zero %rN which
   * also has the effect of resetting flags (at least those recorded
   * by REGS_SAVER_FUNC) to the following:
   *   bit 0: CF=0 (always set to 0 by xor)
   *   bit 2: PF=1 (bottom 8 bits of result have even number of 1s)
   *   bit 6: ZF=1 (set since result of xor is zero)
   *   bit 7: SF=0 (top bit of result)
   *   bit 11: OF=0 (always set to 0 by xor)
   */
  expected_regs->flags = (1 << 2) | (1 << 6);
#endif

#if defined(__i386__)
  /* The current implementation sets %edx to the return address. */
  expected_regs->edx = expected_regs->prog_ctr;
#elif defined(__x86_64__)
  /*
   * The current implementation sets %r11 to the %r15-extended return
   * address.  Note that sandbox-base-address-hiding relies on not
   * leaking this 64-bit address into any other register.
   */
  expected_regs->r11 = expected_regs->prog_ctr;
  /* NaCl always sets %rdi (argument 1) as if it is calling _start. */
  expected_regs->rdi = (uintptr_t) expected_regs->stack_ptr + 8;
#elif defined(__arm__)
  /* The current implementation sets r1 to the return address. */
  expected_regs->r1 = expected_regs->prog_ctr;
  /*
   * When starting a new thread, NaCl sets r0 (argument 1) as if it is
   * calling _start.
   */
  expected_regs->r0 = expected_regs->stack_ptr;
#elif defined(__mips__)
  /* The current implementation sets t9 to the return address. */
  expected_regs->t9 = expected_regs->prog_ctr;
  /*
   * Value from the return register v0 is put in input register a0 when
   * a new thread is started. For the start of a new thread, expected
   * stack pointer value is put in these registers.
   */
  expected_regs->a0 = expected_regs->stack_ptr;
  expected_regs->v0 = expected_regs->stack_ptr;
#endif
}


REGS_SAVER_FUNC(ContinueAfterSyscall, CheckRegistersAfterSyscall);

void CheckRegistersAfterSyscall(struct NaClSignalContext *regs) {
  /* Ignore the register containing the return value. */
#if defined(__i386__)
  g_expected_regs.eax = regs->eax;
#elif defined(__x86_64__)
  g_expected_regs.rax = regs->rax;
#elif defined(__arm__)
  g_expected_regs.r0 = regs->r0;
#elif defined(__mips__)
  g_expected_regs.v0 = regs->v0;
  /* NaClSwitch sets registers a0 and v0 to the same value. */
  g_expected_regs.a0 = regs->v0;
#else
# error Unsupported architecture
#endif

  RegsAssertEqual(regs, &g_expected_regs);
  longjmp(g_return_jmp_buf, 1);
}

/* This tests a NaCl syscall that takes no arguments. */
void TestSyscall(uintptr_t syscall_addr) {
  struct NaClSignalContext call_regs;
  char stack[0x10000];

  RegsFillTestValues(&call_regs, /* seed= */ 0);
  call_regs.stack_ptr = (uintptr_t) stack + sizeof(stack);
  call_regs.prog_ctr = (uintptr_t) ContinueAfterSyscall;
  RegsApplySandboxConstraints(&call_regs);

  g_expected_regs = call_regs;
  RegsUnsetNonCalleeSavedRegisters(&g_expected_regs);
  SetNaClSwitchExpectations(&g_expected_regs);

  if (!setjmp(g_return_jmp_buf)) {
#if defined(__i386__)
    call_regs.eax = syscall_addr;
    ASM_WITH_REGS(
        &call_regs,
        "push $ContinueAfterSyscall\n"  /* Push return address */
        "nacljmp %%eax\n");
#elif defined(__x86_64__)
    /*
     * This fast path syscall happens to preserve various registers,
     * but that is obviously not guaranteed by the ABI.
     */
    if (syscall_addr == (uintptr_t) NACL_SYSCALL(tls_get) ||
        syscall_addr == (uintptr_t) NACL_SYSCALL(second_tls_get)) {
      /* Undo some effects of RegsUnsetNonCalleeSavedRegisters(). */
      g_expected_regs.rsi = call_regs.rsi;
      g_expected_regs.rdi = call_regs.rdi;
      g_expected_regs.r8 = call_regs.r8;
      g_expected_regs.r9 = call_regs.r9;
      g_expected_regs.r10 = call_regs.r10;
      /*
       * The current implementation clobbers %rcx with the
       * non-%r15-extended return address.
       */
      g_expected_regs.rcx = (uint32_t) g_expected_regs.prog_ctr;
    }

    call_regs.rax = syscall_addr;
    ASM_WITH_REGS(
        &call_regs,
        "push $ContinueAfterSyscall\n"  /* Push return address */
        "nacljmp %%eax, %%r15\n");
#elif defined(__arm__)
    call_regs.r1 = syscall_addr;  /* Scratch register */
    call_regs.lr = (uintptr_t) ContinueAfterSyscall;  /* Return address */
    ASM_WITH_REGS(
        &call_regs,
        "bic r1, r1, #0xf000000f\n"
        "bx r1\n");
#elif defined(__mips__)
    call_regs.t9 = syscall_addr;  /* Scratch register */
    call_regs.return_addr = (uintptr_t) ContinueAfterSyscall;  /* Return */
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


REGS_SAVER_FUNC(ThreadFuncWrapper, ThreadFunc);

void ThreadFunc(struct NaClSignalContext *regs) {
  g_initial_thread_regs = *regs;
  ANNOTATE_CONDVAR_SIGNAL(&g_stack_in_use);
  NACL_SYSCALL(thread_exit)((int32_t *) &g_stack_in_use);
  assert(!"Should not reach here");
}

/*
 * This tests that when a new thread is created, untrusted code is
 * entered with well-defined register state.  None of the registers
 * should come from uninitialised values.
 */
void TestInitialRegsAtThreadEntry(void) {
  char *stack_top = g_stack + sizeof(g_stack);
  uintptr_t aligned_stack_top =
      ((uintptr_t) stack_top & ~NACL_STACK_ALIGN_MASK)
      - NACL_STACK_PAD_BELOW_ALIGN;
  /*
   * We do not care about TLS for this test, but sel_ldr rejects a
   * zero tls argument, so use an arbitrary non-zero value.
   */
  char *tls = (char *) 0x1000;
  g_stack_in_use = 1;
  int rc = NACL_SYSCALL(thread_create)((void *) (uintptr_t) ThreadFuncWrapper,
                                       stack_top, tls, 0);
  assert(rc == 0);
  /* Spin until the thread exits. */
  while (g_stack_in_use) {
    sched_yield();
  }
  ANNOTATE_CONDVAR_WAIT(&g_stack_in_use);
  struct NaClSignalContext actual_regs = g_initial_thread_regs;

  struct NaClSignalContext expected_regs;
  /* By default, we expect registers to be initialised to zero. */
  memset(&expected_regs, 0, sizeof(expected_regs));
  expected_regs.prog_ctr = (uintptr_t) ThreadFuncWrapper;
  expected_regs.stack_ptr = aligned_stack_top - NACL_STACK_ARGS_SIZE;
  RegsApplySandboxConstraints(&expected_regs);
  SetNaClSwitchExpectations(&expected_regs);
#if defined(__x86_64__)
  /* NaCl happens to initialise %rbp to be the same as %rsp. */
  expected_regs.rbp = expected_regs.stack_ptr;
#endif

  RegsAssertEqual(&actual_regs, &expected_regs);
}


int main(void) {
  printf("Testing null syscall...\n");
  TestSyscall((uintptr_t) NACL_SYSCALL(null));

  /*
   * Check tls_get and second_tls_get specifically because they are
   * implemented via fast paths in assembly code.
   */
  printf("Testing tls_get syscall...\n");
  TestSyscall((uintptr_t) NACL_SYSCALL(tls_get));

  printf("Testing second_tls_get syscall...\n");
  TestSyscall((uintptr_t) NACL_SYSCALL(second_tls_get));

  printf("Testing initial register state at thread entry...\n");
  TestInitialRegsAtThreadEntry();

  return 0;
}
