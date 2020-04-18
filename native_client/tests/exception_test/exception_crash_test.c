/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_test_crash.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"


char stack_in_rwdata[0x1000];

/*
 * Note that we have to provide an initialiser here otherwise gcc
 * defines this using ".comm" and the variable does not get put into a
 * read-only segment.
 */
const char stack_in_rodata[0x1000] = "blah";


void test_bad_handler(void) {
  /*
   * Use an address that we know contains no valid code, yet is within
   * the code segment range and is well-aligned.  The bottom 64k of
   * address space is never mapped.
   */
  nacl_exception_handler_t handler = (nacl_exception_handler_t) 0x1000;
  int rc = nacl_exception_set_handler(handler);
  assert(rc == 0);
  fprintf(stderr, "** intended_exit_status=untrusted_segfault\n");
  /* Cause crash. */
  *(volatile int *) 0 = 0;
}


#if defined(__i386__) || defined(__x86_64__)

/*
 * If there are TCB bugs, it is possible that the untrusted exception
 * handler gets run on a bad stack, either because the TCB ignored the
 * error it got when writing the exception frame, or because page
 * protections were ignored when writing the exception frame.  (The
 * latter is a problem with WriteProcessMemory() on Windows: see
 * http://code.google.com/p/nativeclient/issues/detail?id=2536.)
 *
 * If that happens, the test needs to report the problem.  In order to
 * do that, our exception handler needs to restore a working stack.
 */

char recovery_stack[0x1000] __attribute__((aligned(16)));

void bad_stack_exception_handler(struct NaClExceptionContext *context);
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".p2align 5\n"
        "bad_stack_exception_handler:\n"
        /* Restore a working stack, allowing for alignment. */
# if defined(__i386__)
        "mov $recovery_stack - 4, %esp\n"
        "jmp error_exit\n"
# elif defined(__x86_64__)
        "naclrestsp $recovery_stack - 8, %r15\n"
        "jmp error_exit\n"
# endif
        ".popsection\n");

void error_exit(void) {
  _exit(1);
}

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

char recovery_stack[0x1000] __attribute__((aligned(8)));

void bad_stack_exception_handler(struct NaClExceptionContext *context);
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".set noreorder\n"
        ".global bad_stack_exception_handler\n"
        "bad_stack_exception_handler:\n"
        "lui   $t9,      %hi(recovery_stack)\n"
        "addiu $sp, $t9, %lo(recovery_stack)\n"
        "and   $sp, $sp, $t7\n"
        "lui   $t9,      %hi(error_exit)\n"
        "j error_exit\n"
        "addiu $t9, $t9, %lo(error_exit)\n"
        "nop\n"
        "nop\n"
        ".set reorder\n"
        ".popsection\n");

void error_exit() {
  _exit(1);
}

#else

/* TODO(mseaborn): Implement a stack switcher, like the one above, for ARM. */
void bad_stack_exception_handler(struct NaClExceptionContext *context) {
  _exit(1);
}

#endif


/*
 * This checks that the process terminates safely if the system
 * attempts to write a stack frame at the current stack pointer when
 * the stack pointer points outside of the sandbox's address space.
 *
 * This only applies to x86-32, because the stack pointer register
 * cannot be set to point outside of the sandbox's address space on
 * x86-64 and ARM.
 */
void test_stack_outside_sandbox(void) {
#if defined(__i386__)
  int rc = nacl_exception_set_handler(bad_stack_exception_handler);
  assert(rc == 0);
  fprintf(stderr, "** intended_exit_status=untrusted_segfault\n");
  __asm__(
      /*
       * Set the stack pointer to an address that is definitely
       * outside the sandbox's address space.
       */
      "movl $0xffffffff, %esp\n"
      /* Cause crash. */
      "movl $0, 0\n");
#else
  fprintf(stderr, "test_bad_stack does not apply on this platform\n");
  fprintf(stderr, "** intended_exit_status=0\n");
  exit(0);
#endif
}


/*
 * This test case does not crash.  It successfully runs
 * bad_stack_exception_handler() in order to check that it works, so
 * that we can be sure that other tests do not crash (and hence pass)
 * accidentally.
 */
void test_stack_in_rwdata(void) {
  int rc = nacl_exception_set_handler(bad_stack_exception_handler);
  assert(rc == 0);
  rc = nacl_exception_set_stack((void *) stack_in_rwdata,
                                sizeof(stack_in_rwdata));
  assert(rc == 0);
  fprintf(stderr, "** intended_exit_status=1\n");
  /* Cause crash. */
  *(volatile int *) 0 = 0;
}


/*
 * This reproduced a problem with NaCl's exception handler on Mac OS
 * X, in which sel_ldr would hang when attempting to write to an
 * unwritable stack.
 */
void test_stack_in_rodata(void) {
  int rc = nacl_exception_set_handler(bad_stack_exception_handler);
  assert(rc == 0);
  rc = nacl_exception_set_stack((void *) stack_in_rodata,
                                sizeof(stack_in_rodata));
  assert(rc == 0);
  fprintf(stderr, "** intended_exit_status=unwritable_exception_stack\n");
  /* Cause crash. */
  *(volatile int *) 0 = 0;
}


/*
 * The test below reproduced a problem with NaCl's exception handler
 * on Windows where WriteProcessMemory() would bypass page permissions
 * and write the exception frame into the code area.
 */
#if defined(__i386__) || defined(__x86_64__)
/*
 * If we have an assembler, we can reserve some of the code area so
 * that we do not risk overwriting bad_stack_exception_handler() if
 * the test fails.
 *
 * This might be in the static or dynamic code area depending on
 * whether this executable is statically or dynamically linked, and
 * the two areas potentially behave differently.  Therefore, for full
 * coverage, this test should be run as both statically and
 * dynamically linked.
 */
__asm__(".pushsection .text, \"ax\", @progbits\n"
        "stack_in_code:\n"
        ".fill 0x1000, 1, 0x90\n" /* Fill with NOPs (90) */
        ".popsection\n");
extern char stack_in_code[];
#else
/* Otherwise just use the start of the static code area. */
char *stack_in_code = (char *) 0x20000;
#endif
const int stack_in_code_size = 0x1000;

void test_stack_in_code(void) {
  int rc = nacl_exception_set_handler(bad_stack_exception_handler);
  assert(rc == 0);
  rc = nacl_exception_set_stack(stack_in_code, stack_in_code_size);
  assert(rc == 0);
  fprintf(stderr, "** intended_exit_status=unwritable_exception_stack\n");
  /* Cause crash. */
  *(volatile int *) 0 = 0;
}


/*
 * This checks that crashes in trusted code (such as inside NaCl
 * syscalls) do not cause the untrusted exception handler to run.
 */
void test_crash_in_syscall(void) {
  int rc = nacl_exception_set_handler(bad_stack_exception_handler);
  assert(rc == 0);
  rc = nacl_exception_set_stack((void *) stack_in_rwdata,
                                sizeof(stack_in_rwdata));
  assert(rc == 0);
  fprintf(stderr, "** intended_exit_status=trusted_segfault\n");
  /*
   * Cause a crash inside a NaCl syscall.
   */
  NACL_SYSCALL(test_crash)(NACL_TEST_CRASH_MEMORY);
  /* Should not reach here. */
  _exit(1);
}


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: program <test-name>\n");
    return 1;
  }

#define TRY_TEST(test_name) \
    if (strcmp(argv[1], #test_name) == 0) { test_name(); return 1; }

  TRY_TEST(test_bad_handler);
  TRY_TEST(test_stack_outside_sandbox);
  TRY_TEST(test_stack_in_rwdata);
  TRY_TEST(test_stack_in_rodata);
  TRY_TEST(test_stack_in_code);
  TRY_TEST(test_crash_in_syscall);

  fprintf(stderr, "Error: Unknown test: \"%s\"\n", argv[1]);
  return 1;
}
