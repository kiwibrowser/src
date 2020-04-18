/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/elf_constants.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/tests/common/register_set.h"
#include "native_client/tests/inbrowser_test_runner/test_runner.h"


/*
 * This is used for calculating the size of the exception stack frame
 * when alignment is taken into account.
 */
struct CombinedContext {
  struct NaClExceptionContext c1;
  struct NaClExceptionPortableContext c2;
};

char stack[0x10000];

struct NaClSignalContext g_regs_at_crash;
jmp_buf g_jmp_buf;

char *g_registered_stack;
size_t g_registered_stack_size;


#if defined(__mips__)
#define STACK_ALIGNMENT 8
#elif defined(__arm__) && defined(__native_client_nonsfi__)
/* AAPCS stack alignment, for Non-SFI NaCl. */
#define STACK_ALIGNMENT 8
#else
/* NaCl stack alignment. */
#define STACK_ALIGNMENT 16
#endif

#if defined(__i386__)
const int kReturnAddrSize = 4;
const int kArgSizeOnStack = 4;
const int kRedZoneSize = 0;
#elif defined(__x86_64__)
const int kReturnAddrSize = 8;
const int kArgSizeOnStack = 0;
const int kRedZoneSize = 128;
#elif defined(__arm__)
const int kReturnAddrSize = 0;
const int kArgSizeOnStack = 0;
const int kRedZoneSize = 0;
#elif defined(__mips__)
const int kReturnAddrSize = 0;
const int kArgSizeOnStack = 16;
const int kRedZoneSize = 0;
#else
# error Unsupported architecture
#endif

void crash_at_known_address(void);
extern char prog_ctr_at_crash[];
#if defined(__i386__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".p2align 5\n"
        "crash_at_known_address:\n"
        "prog_ctr_at_crash:\n"
        "movl $0, 0\n"
        ".popsection");
#elif defined(__x86_64__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".p2align 5\n"
        "crash_at_known_address:\n"
        "prog_ctr_at_crash:\n"
        "movl $0, (%r15)\n"
        ".popsection");
#elif defined(__arm__)
__asm__(".pushsection .text, \"ax\", %progbits\n"
        ".p2align 4\n"
        "crash_at_known_address:\n"
        "mov r0, #0\n"
        SFI_OR_NONSFI_CODE("bic r0, r0, #0xc0000000\n", "")
        "prog_ctr_at_crash:\n"
        "str r0, [r0]\n"
        ".popsection\n");
#elif defined(__mips__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".p2align 4\n"
        ".global crash_at_known_address\n"
        "crash_at_known_address:\n"
        ".global prog_ctr_at_crash\n"
        "prog_ctr_at_crash:\n"
        "break\n"
        ".popsection\n");
#else
# error Unsupported architecture
#endif


void return_from_exception_handler(void) {
  /*
   * Clear the exception flag so that future faults will invoke the
   * exception handler.
   */
  int rc = nacl_exception_clear_flag();
  assert(rc == 0);
  longjmp(g_jmp_buf, 1);
}

void simple_exception_handler(struct NaClExceptionContext *regs) {
  return_from_exception_handler();
}

void exception_handler(struct NaClExceptionContext *context);
REGS_SAVER_FUNC_NOPROTO(exception_handler, exception_handler_wrapped);

void exception_handler_wrapped(struct NaClSignalContext *entry_regs) {
  struct NaClExceptionContext *context =
      (struct NaClExceptionContext *) RegsGetArg1(entry_regs);
  struct NaClExceptionPortableContext *portable =
      nacl_exception_context_get_portable(context);

  printf("handler called\n");

  assert(context->size == (uintptr_t) (portable + 1) - (uintptr_t) context);
  assert(context->portable_context_size ==
         sizeof(struct NaClExceptionPortableContext));
  assert(context->regs_size == sizeof(NaClUserRegisterState));

  assert(portable->stack_ptr == (uint32_t) g_regs_at_crash.stack_ptr);
  assert(portable->prog_ctr == (uintptr_t) prog_ctr_at_crash);
#if defined(__i386__)
  assert(portable->frame_ptr == g_regs_at_crash.ebp);
  assert(context->arch == EM_386);
#elif defined(__x86_64__)
  assert(portable->frame_ptr == (uint32_t) g_regs_at_crash.rbp);
  assert(context->arch == EM_X86_64);
#elif defined(__arm__)
  assert(portable->frame_ptr == g_regs_at_crash.r11);
  assert(context->arch == EM_ARM);
#  if !defined(__native_client_nonsfi__)
  /* Verify we have scrubbed r9 for NaCl arm. */
  assert(context->regs.r9 == -1);
#  endif
#elif defined(__mips__)
  assert(portable->frame_ptr == g_regs_at_crash.frame_ptr);
  assert(context->arch == EM_MIPS);
#else
# error Unsupported architecture
#endif

  /*
   * Convert the NaClUserRegisterState to a NaClSignalContext so that
   * we can reuse RegsAssertEqual() to compare the register state.
   */
  struct NaClSignalContext reported_regs;
  RegsCopyFromUserRegisterState(&reported_regs, &context->regs);
  RegsAssertEqual(&reported_regs, &g_regs_at_crash);

  const int kMaxStackFrameSize = 0x1000;
  char *stack_top;
  char local_var;
  if (g_registered_stack == NULL) {
    /* Check that our current stack is just below the saved stack pointer. */
    stack_top = (char *) portable->stack_ptr - kRedZoneSize;
    assert(stack_top - kMaxStackFrameSize < &local_var);
    assert(&local_var < stack_top);
  } else {
    /* Check that we are running on the exception stack. */
    stack_top = g_registered_stack + g_registered_stack_size;
    assert(g_registered_stack <= &local_var);
    assert(&local_var < stack_top);
  }

  /* Check the exception handler's initial stack pointer more exactly. */
  uintptr_t frame_base = entry_regs->stack_ptr + kReturnAddrSize;
  assert(frame_base % STACK_ALIGNMENT == 0);
  char *frame_top = (char *) (frame_base + kArgSizeOnStack +
                              sizeof(struct CombinedContext));
#if defined(__native_client_nonsfi__)
  /*
   * Non-SFI mode exception stack will have ucontext_t and
   * siginfo_t, which would be anywhere from 400 bytes to 1.5k
   */
  assert(stack_top - 1500 < frame_top);
#else
  /* Check that no more than the stack alignment size is wasted. */
  assert(stack_top - STACK_ALIGNMENT < frame_top);
#endif

  assert(frame_top <= stack_top);

#if defined(__x86_64__)
  /* Check that %rsp and %rbp have safe, %r15-extended values. */
  assert(entry_regs->stack_ptr >> 32 == entry_regs->r15 >> 32);
  assert(entry_regs->rbp >> 32 == entry_regs->r15 >> 32);
#endif

  return_from_exception_handler();
}

void test_exception_stack_with_size(char *stack, size_t stack_size) {
  if (0 != nacl_exception_set_handler(exception_handler)) {
    printf("failed to set exception handler\n");
    exit(4);
  }
#if !defined(__native_client_nonsfi__)
  /* TODO(uekawa): Implement set_stack for Non-SFI mode. */
  if (0 != nacl_exception_set_stack(stack, stack_size)) {
    printf("failed to set alt stack\n");
    exit(5);
  }
  g_registered_stack = stack;
  g_registered_stack_size = stack_size;
#endif

  char crash_stack[0x1000];
  RegsFillTestValues(&g_regs_at_crash, /* seed= */ 0);
  g_regs_at_crash.stack_ptr = (uintptr_t) crash_stack + sizeof(crash_stack);
  g_regs_at_crash.prog_ctr = (uintptr_t) prog_ctr_at_crash;
  RegsApplySandboxConstraints(&g_regs_at_crash);
#if defined(__arm__)
  /* crash_at_known_address clobbers r0. */
  g_regs_at_crash.r0 = 0;
#endif

  if (!setjmp(g_jmp_buf)) {
    JUMP_WITH_REGS(&g_regs_at_crash, crash_at_known_address);
  }
  /* Clear the jmp_buf to prevent it from being reused accidentally. */
  memset(g_jmp_buf, 0, sizeof(g_jmp_buf));
}

void test_exceptions_minimally(void) {
  /* Test exceptions without having an exception stack set up. */
  test_exception_stack_with_size(NULL, 0);

  test_exception_stack_with_size(stack, sizeof(stack));
}

void test_exception_stack_alignments(void) {
  /*
   * Test the stack realignment logic by trying stacks which end at
   * different addresses modulo the stack alignment size.
   */
  int diff;
  for (diff = 0; diff <= STACK_ALIGNMENT * 2; diff++) {
    test_exception_stack_with_size(stack, sizeof(stack) - diff);
  }
}

void test_getting_previous_handler(void) {
  int rc;
  nacl_exception_handler_t prev_handler;

  rc = nacl_exception_get_and_set_handler(exception_handler, NULL);
  assert(rc == 0);

  rc = nacl_exception_get_and_set_handler(NULL, &prev_handler);
  assert(rc == 0);
  assert(prev_handler == exception_handler);

  rc = nacl_exception_get_and_set_handler(NULL, &prev_handler);
  assert(rc == 0);
  assert(prev_handler == NULL);
}

void test_invalid_handlers(void) {
  int rc;
  nacl_exception_handler_t unaligned_func_ptr =
    (nacl_exception_handler_t) ((uintptr_t) exception_handler + 1);
  const char *ptr_in_rodata_segment = "";

  /* An alignment check is required for safety in all NaCl sandboxes. */
  rc = nacl_exception_set_handler(unaligned_func_ptr);
  assert(rc == EFAULT);

  /* A range check is required for safety in the NaCl ARM sandbox. */
  rc = nacl_exception_set_handler(
      (nacl_exception_handler_t) (uintptr_t) ptr_in_rodata_segment);
  assert(rc == EFAULT);
}


void *thread_func(void *unused_arg) {
  /*
   * This tests that the exception handler gets the correct
   * NaClAppThread for this thread.  If it gets the wrong thread, the
   * handler will detect that the stack it is running on does not
   * match the stack that was registered.
   *
   * On x86-64 Windows, it is non-trivial for an out-of-process
   * exception handler to determine the NaClAppThread for a faulting
   * thread, which is why we need a test for this.
   */
  test_exception_stack_with_size(NULL, 0);
  test_exception_stack_with_size(stack, sizeof(stack));
  return NULL;
}

void test_exceptions_on_non_main_thread(void) {
  pthread_t tid;
  int rc = pthread_create(&tid, NULL, thread_func, NULL);
  assert(rc == 0);
  rc = pthread_join(tid, NULL);
  assert(rc == 0);
}

void test_catching_various_exception_types(void) {
  int rc = nacl_exception_set_handler(simple_exception_handler);
  assert(rc == 0);

  printf("Testing __builtin_trap()...\n");
  if (!setjmp(g_jmp_buf)) {
    __builtin_trap();
    exit(1);
  }

#if defined(__i386__) || defined(__x86_64__)
  printf("Testing hlt...\n");
  if (!setjmp(g_jmp_buf)) {
    __asm__("hlt");
    exit(1);
  }
  printf("Testing ud2a (an illegal instruction)...\n");
  if (!setjmp(g_jmp_buf)) {
    __asm__("ud2a");
    exit(1);
  }
  printf("Testing integer division by zero...\n");
  if (!setjmp(g_jmp_buf)) {
    uint32_t result;
    __asm__ volatile("idivb %1"
                     : "=a"(result)
                     : "r"((uint8_t) 0), "a"((uint16_t) 1));
    exit(1);
  }
#endif

  /* Clear the jmp_buf to prevent it from being reused accidentally. */
  memset(g_jmp_buf, 0, sizeof(g_jmp_buf));
}


#if defined(__i386__) || defined(__x86_64__)

int get_x86_direction_flag(void);

void test_get_x86_direction_flag(void) {
  /*
   * Sanity check:  Ensure that get_x86_direction_flag() works.  We
   * avoid calling assert() with the flag set, because that might not
   * work.
   */
  assert(get_x86_direction_flag() == 0);
  __asm__("std");
  int flag = get_x86_direction_flag();
  __asm__("cld");
  assert(flag == 1);
}

void direction_flag_exception_handler(struct NaClExceptionContext *context) {
  assert(get_x86_direction_flag() == 0);
  return_from_exception_handler();
}

/*
 * The x86 ABIs require that the x86 direction flag is unset on entry
 * to a function.  However, a crash could occur while the direction
 * flag is set.  In order for an exception handler to be a normal
 * function without x86-specific knowledge, NaCl needs to unset the
 * direction flag when calling the exception handler.  This test
 * checks that this happens.
 */
void test_unsetting_x86_direction_flag(void) {
  int rc = nacl_exception_set_handler(direction_flag_exception_handler);
  assert(rc == 0);
  if (!setjmp(g_jmp_buf)) {
    /* Cause a crash with the direction flag set. */
    __asm__("std");
    *((volatile int *) 0) = 0;
    /* Should not reach here. */
    exit(1);
  }
  /* Clear the jmp_buf to prevent it from being reused accidentally. */
  memset(g_jmp_buf, 0, sizeof(g_jmp_buf));
}

#endif


void run_test(const char *test_name, void (*test_func)(void)) {
  printf("Running %s...\n", test_name);
  test_func();
}

#define RUN_TEST(test_func) (run_test(#test_func, test_func))

int TestMain(void) {
  RUN_TEST(test_exceptions_minimally);
  RUN_TEST(test_getting_previous_handler);
#if !defined(__native_client_nonsfi__)
  /* TODO(uekawa): Implement set_stack for Non-SFI mode. */
  RUN_TEST(test_exception_stack_alignments);
  /* Those handlers are not invalid in NonSFI NaCl. */
  RUN_TEST(test_invalid_handlers);
#endif
  /* pthread_join() is broken under qemu-arm. */
  if (getenv("UNDER_QEMU_ARM") == NULL)
    RUN_TEST(test_exceptions_on_non_main_thread);
  RUN_TEST(test_catching_various_exception_types);

#if defined(__i386__) || defined(__x86_64__)
  RUN_TEST(test_get_x86_direction_flag);
  RUN_TEST(test_unsetting_x86_direction_flag);
#endif

  fprintf(stderr, "** intended_exit_status=0\n");
  return 0;
}

int main(void) {
  return RunTests(TestMain);
}
