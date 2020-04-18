/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_assert.h"
#include "native_client/tests/common/register_set.h"


/* This variable is used for testing memory accesses. */
char g_example_var[] = "some_debug_stub_test_data";

volatile uint32_t g_main_thread_var = 0;
volatile uint32_t g_child_thread_var = 0;


/*
 * Inline assembly is not allowed to define symbols (in case it gets
 * instantiated multiple times), so we must define this symbol using
 * top-level assembly.
 */
#if defined(__i386__) || defined(__x86_64__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        "fault_addr:\n"
        "hlt\n"
        ".popsection\n");
#elif defined(__arm__)
__asm__(".pushsection .text, \"ax\", %progbits\n"
        "fault_addr:\n"
        ".word " NACL_TO_STRING(NACL_INSTR_ARM_ABORT_NOW) "\n"
        ".popsection\n");
#elif defined(__mips__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        "fault_addr:\n"
        ".word 0x0000000d\n" /* Break instruction on MIPS. */
        ".popsection\n");
#else
# error Update fault_addr for other architectures
#endif

void set_registers_and_stop(void) {
  struct NaClSignalContext regs;
  memset(&regs, 0, sizeof(regs));

  /*
   * We set most registers to fixed values before faulting, so that we
   * can test that the debug stub successfully returns the same
   * values.
   */
#if defined(__i386__)
  regs.eax = 0x11000022;
  regs.ebx = 0x22000033;
  regs.ecx = 0x33000044;
  regs.edx = 0x44000055;
  regs.esi = 0x55000066;
  regs.edi = 0x66000077;
  regs.ebp = 0x77000088;
  regs.stack_ptr = 0x88000099;
#elif defined(__x86_64__)
  regs.rax = 0x1100000000000022;
  regs.rbx = 0x2200000000000033;
  regs.rcx = 0x3300000000000044;
  regs.rdx = 0x4400000000000055;
  regs.rsi = 0x5500000000000066;
  regs.rdi = 0x6600000000000077;
  regs.r8  = 0x7700000000000088;
  regs.r9  = 0x8800000000000099;
  regs.r10 = 0x99000000000000aa;
  regs.r11 = 0xaa000000000000bb;
  regs.r12 = 0xbb000000000000cc;
  regs.r13 = 0xcc000000000000dd;
  regs.r14 = 0xdd000000000000ee;
  /*
   * These stack pointer test values need to be 32-bit, since the r15
   * base address gets added to them.
   */
  regs.stack_ptr = 0x12300321;
  regs.rbp = 0x23400432;
#elif defined(__arm__)
  regs.r0 = 0x00000001;
  regs.r1 = 0x10000002;
  regs.r2 = 0x20000003;
  regs.r3 = 0x30000004;
  regs.r4 = 0x40000005;
  regs.r5 = 0x50000006;
  regs.r6 = 0x60000007;
  regs.r7 = 0x70000008;
  regs.r8 = 0x80000009;
  /*
   * Skip r9 because it is not supposed to be settable or readable by
   * untrusted code.
   */
  regs.r10 = 0xa000000b;
  regs.r11 = 0xb000000c;
  regs.r12 = 0xc000000d;
  /*
   * stack_ptr's test value must be within the sandbox address space.
   * However we can test that we mask sp before allowing access to it.
   */
  regs.stack_ptr = 0xD2345678;
  regs.lr = 0xe000000f;
  regs.cpsr = (1 << 29); /* C flag */
#elif defined(__mips__)
  /* Skip zero register because it cannot be set. */
  regs.at = 0x11000220;
  regs.v0 = 0x22000330;
  regs.v1 = 0x33000440;
  regs.a0 = 0x44000550;
  regs.a1 = 0x55000660;
  regs.a2 = 0x66000770;
  regs.a3 = 0x77000880;
  regs.t0 = 0x88000990;
  regs.t1 = 0x99000aa0;
  regs.t2 = 0xaa000bb0;
  regs.t3 = 0xbb000cc0;
  regs.t4 = 0xcc000dd0;
  regs.t5 = 0xdd000ee0;
  /* Skip t6, t7 and t8, because they cannot be set by untrusted code. */
  regs.s0 = 0x11100222;
  regs.s1 = 0x22200333;
  regs.s2 = 0x33300444;
  regs.s3 = 0x44400555;
  regs.s4 = 0x55500666;
  regs.s5 = 0x66600777;
  regs.s6 = 0x77700888;
  regs.s7 = 0x88800999;
  regs.t9 = 0xaaa00bbb;
  /* Skip k0 and k1 registers, since they can be changed by kernel. */
  regs.global_ptr = 0xddd00eee;
  regs.stack_ptr = 0x2ee00fff;
  regs.frame_ptr = 0xfff00000;
  regs.return_addr = 0x0a0a0a0a;
#else
# error Update set_registers_and_stop for other architectures
#endif
  JUMP_WITH_REGS(&regs, fault_addr);
}

void test_jump_to_address_zero(void) {
  /*
   * "volatile" tells Clang/LLVM not to optimize this call away and
   * replace it with an illegal instruction, which produces different
   * behaviour (SIGILL rather than SIGSEGV).
   */
  void (*volatile null_ptr)(void) = NULL;
  null_ptr();
}

/*
 * The test sets a breakpoint on this function, so for that to work
 * this function must not be inlined.
 */
__attribute__((noinline))
void breakpoint_target_func(void) {
  /*
   * This is also necessary to prevent inlining according to the GCC
   * docs for "noinline".
   */
  __asm__("");
}

int non_zero_return(void) {
  return 2;
}

void test_single_step(void) {
  /*
   * The actual instructions used here don't matter, we just want to use
   * a variety of instruction sizes.
   * Instruction bytes are specified explicitly to avoid ambiguous encodings
   * and thus ensure required instructions sizes.
   */
#if defined(__i386__)
  __asm__(
      ".byte 0xf4\n"                                /* hlt */
      ".byte 0x53\n"                                /* push %ebx */
      ".byte 0x39, 0xd8\n"                          /* cmp  %ebx,%eax */
      ".byte 0x83, 0xeb, 0x01\n"                    /* sub  $0x1,%ebx */
      ".byte 0x81, 0xeb, 0x14, 0x9f, 0x04, 0x08\n"  /* sub  $0x8049f14,%ebx */
      ".byte 0x5b\n");                              /* pop  %ebx */
#elif defined(__x86_64__)
  __asm__(
      ".byte 0xf4\n"                                /* hlt */
      ".byte 0x53\n"                                /* push %rbx */
      ".byte 0x48, 0x39, 0xd8\n"                    /* cmp  %rbx,%rax */
      ".byte 0x48, 0x83, 0xeb, 0x01\n"              /* sub  $0x1,%rbx */
      ".byte 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00\n"  /* nopw 0x0(%rax,%rax,1) */
      ".byte 0x5b\n");                              /* pop  %rbx */
#elif defined(__arm__) || defined(__mips__)
  printf("Single-stepping is not supported on ARM and MIPS.\n");
  exit(1);
#else
# error Update test_single_step for other architectures
#endif
}

void test_interrupt(void) {
  /*
   * 'volatile' is needed for clang optimizer to get this right.
   * TODO(robertm): http://code.google.com/p/nativeclient/issues/detail?id=2912
   */
  volatile int x = 0;
  for (;;) {
    x = (x + 1) % 2;
  }
}

void breakpoint(void) {
#if defined(__i386__) || defined(__x86_64__)
  /* We avoid using int3 because of a Mac OS X kernel bug. */
  __asm__("hlt");
#elif defined(__arm__)
  /*
   * Arrange the breakpoint so that the test can skip over it by
   * jumping to the next bundle.  This means we never have to set the
   * program counter to within a bundle, which could be unsafe,
   * because BKPTs guard data literals in the ARM sandbox.
   */
  __asm__(".p2align 4\n"
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_ABORT_NOW) "\n"
          ".p2align 4\n");
#elif defined(__mips__)
  __asm__(".p2align 4\n"
          ".word 0x0000000d\n" /* Break instruction on MIPS. */
          ".p2align 4\n");
#else
# error Unsupported architecture
#endif
}

void *child_thread_func(void *thread_arg) {
  for (;;) {
    g_child_thread_var++;
    breakpoint();
  }
  return NULL;
}

void test_suspending_threads(void) {
  pthread_t tid;
  ASSERT_EQ(pthread_create(&tid, NULL, child_thread_func, NULL), 0);
  for (;;) {
    g_main_thread_var++;
  }
}

/*
 * Single super-instruction to test register constraints.
 * Tests also assume the super-instruction immediately follows the hlt.
 */
void test_super_instruction(void) {
#if defined(__i386__)
  __asm__(".p2align 5\n" /* Ensures nacljmp doesn't cross a bundle boundary */
          "hlt\n"
          "nacljmp %ecx\n");
#elif defined(__x86_64__)
  __asm__(".p2align 5\n" /* Ensures nacljmp doesn't cross a bundle boundary */
          "hlt\n"
          "nacljmp %ecx, %r15");
#elif defined(__arm__)
  __asm__(".p2align 4\n" /* Ensures branch doesn't cross a bundle boundary */
          /*
           * Arrange the breakpoint so that the test can skip over it by
           * jumping to the next bundle.  This means we never have to set the
           * program counter to within a bundle, which could be unsafe,
           * because BKPTs guard data literals in the ARM sandbox.
           */
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_ABORT_NOW) "\n"
          ".p2align 4\n"
          "bic r0, #0xc000000F\n"
          "bx r0");
#endif
}

void test_arm_breakpoint(void) {
#if defined(__arm__)
  __asm__(".p2align 4\n"
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_ABORT_NOW) "\n"
          ".p2align 4\n"
          /* Need a code bundle here since we can't step over the breakpoint
           * into a constant pool. So add NOPS.
           */
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_NOP) "\n"
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_NOP) "\n"
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_NOP) "\n"
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_NOP) "\n"
          /* These are the two constant pool markers. */
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_LITERAL_POOL_HEAD) "\n"
          ".p2align 4\n"
          ".word " NACL_TO_STRING(NACL_INSTR_ARM_BREAKPOINT) "\n"
          ".p2align 4\n");
#endif
}

int main(int argc, char **argv) {
  /*
   * This will crash if the entry-point breakpoint has been mishandled such
   * that our argc and argv values are bogus.  This should catch any
   * regression of http://code.google.com/p/nativeclient/issues/detail?id=1730.
   */
  argv[argc] = 0;

  if (argc < 2) {
    printf("Usage: debugger_test.nexe test_name\n");
    return 1;
  }

  if (strcmp(argv[1], "test_getting_registers") == 0) {
    set_registers_and_stop();
    return 0;
  }
  if (strcmp(argv[1], "test_jump_to_address_zero") == 0) {
    test_jump_to_address_zero();
  }
  if (strcmp(argv[1], "test_setting_breakpoint") == 0) {
    breakpoint_target_func();
    return 0;
  }
  if (strcmp(argv[1], "test_exit_code") == 0) {
    return non_zero_return();
  }
  if (strcmp(argv[1], "test_single_step") == 0) {
    test_single_step();
    return 0;
  }
  if (strcmp(argv[1], "test_interrupt") == 0) {
    test_interrupt();
    return 0;
  }
  if (strcmp(argv[1], "test_suspending_threads") == 0) {
    test_suspending_threads();
    return 0;
  }
  if (strcmp(argv[1], "test_super_instruction") == 0) {
    test_super_instruction();
    return 0;
  }
  if (strcmp(argv[1], "test_arm_breakpoint") == 0) {
    test_arm_breakpoint();
    return 0;
  }

  return 1;
}
