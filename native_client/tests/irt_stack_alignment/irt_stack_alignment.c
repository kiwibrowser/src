/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/untrusted/irt/irt.h"


static bool g_enable_block_hook;
static int g_block_hook_calls;
static uintptr_t g_sp_at_call;
static uintptr_t g_sp_before_call;

/*
 * This is the function that the IRT will call directly.
 * It collects its incoming stack pointer and passes it
 * on to the check_stack C function below.
 */
void test_pre_block_hook(void);
asm(".p2align 5\n"
    "test_pre_block_hook:"
    "mov %esp, %eax\n"
    "jmp check_stack");

/*
 * This gets called with the incoming stack pointer from the IRT
 * callback as its argument.  Take a sample for later examination.
 */
__attribute__((regparm(1))) void check_stack(uintptr_t sp) {
  if (g_enable_block_hook) {
    /*
     * The call instruction pushed the return address word,
     * so adjust back to see the SP value at the call site
     * (which is what should be aligned to 16 bytes).
     */
    g_sp_at_call = sp + 4;
    ++g_block_hook_calls;
  }
}

/*
 * The IRT blockhook interface does not permit either hook to be NULL.
 * So this is a no-op function we use as the post-block hook.
 */
static void dummy_block_hook(void) {
}

/*
 * This calls the given function with the stack explicitly misaligned.
 * Thus we can enter the IRT will a misaligned stack and test whether
 * the IRT code realigns the stack before making another call.
 */
static int misaligned_call(int (*func)(void)) {
  int result;
  asm volatile(
      /*
       * Save the real stack pointer so we can get it back before the
       * compiler takes over again.  Both GCC and Clang happily allow "esp"
       * in the clobbers list, but then do not actually pay any attention
       * to that and just carry on with a clobbered stack pointer so
       * everything goes awry.
       */
      "mov %%esp, %%ebx\n"
      /*
       * Align the stack properly to 16 bytes, and then decrement it so
       * it is misaligned by one byte.  A stack aligned to 1 instead of
       * at least 4 has never been allowed by any variant of the x86-32
       * ABI.  But misaligning just by 4 can easily be glossed over by
       * normal code that pushes an odd number of words.  Misaligning by
       * 1 will never be corrected by anything other than explicit stack
       * realignment, but the hardware will cope with misaligned access
       * just fine.
       */
      "andl $-16, %%esp\n"
      "dec %%esp\n"
      "mov %%esp, %[sp]\n"
      "naclcall %[func]\n"
      "mov %%ebx, %%esp\n"
      : "=a" (result),
        [sp] "=m" (g_sp_before_call)
      : [func] "r" (func)
      /*
       * The compiler doesn't know there is a call in that asm, so it
       * thinks it knows what can and can't happen inside.  The explicit
       * register clobbers tell it that the call-clobbered registers
       * will be clobbered by the call.  Without the "memory" clobber,
       * GCC will decide that the stores to g_enable_block_hook are dead
       * and remove them (with volatile it just moves them so they are
       * both after this call, which is no better).
       */
      : "ecx", "edx", "memory");
  return result;
}

int main(void) {
  struct nacl_irt_basic basic;
  struct nacl_irt_blockhook blockhook;
  size_t size = nacl_interface_query(NACL_IRT_BASIC_v0_1,
                                     &basic, sizeof(basic));
  if (size != sizeof(basic)) {
    fprintf(stderr, "IRT query for %s failed (%zu != %zu)\n",
            NACL_IRT_BASIC_v0_1, size, sizeof(basic));
    return 2;
  }
  size = nacl_interface_query(NACL_IRT_BLOCKHOOK_v0_1,
                                     &blockhook, sizeof(blockhook));
  if (size != sizeof(blockhook)) {
    fprintf(stderr, "IRT query for %s failed (%zu != %zu)\n",
            NACL_IRT_BLOCKHOOK_v0_1, size, sizeof(blockhook));
    return 2;
  }

  int rc = blockhook.register_block_hooks(&test_pre_block_hook,
                                          &dummy_block_hook);
  if (rc != 0) {
    fprintf(stderr, "register_block_hooks failed! %s (%d)\n",
            strerror(rc), rc);
    return 3;
  }

  /*
   * Tell the check_stack code (above) to sample the SP just during
   * this call.  Then disable it again before doing anything else
   * (like printf) that might call a block hook again.
   */
  g_enable_block_hook = true;
  rc = misaligned_call(basic.sched_yield);
  g_enable_block_hook = false;

  if (rc != 0) {
    perror("sched_yield");
    return 4;
  }

  if (g_block_hook_calls != 1) {
    fprintf(stderr, "IRT block hook called %d times, expected 1!\n",
          g_block_hook_calls);
    return 5;
  }

  uintptr_t sp_alignment = g_sp_at_call % 16;

  printf("SP before call = %" PRIxPTR "\n", g_sp_before_call);
  printf("SP at call = %" PRIxPTR "\n", g_sp_at_call);
  printf("SP alignment %% 16 = %" PRIuPTR "\n", sp_alignment);
  return sp_alignment == 0 ? 0 : 1;
}
