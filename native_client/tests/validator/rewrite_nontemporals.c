/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/nacl_assert.h"

/*
 * Require 16-byte alignment because some of the instructions used
 * below require it.  This includes movdqa and movntdq; the latter is
 * rewritten to movdqa.
 */
char g_src[16] __attribute__((aligned(16)));
char g_dest[16] __attribute__((aligned(16)));

void reset_test_vars(void) {
  memset(g_src, 0xff, sizeof(g_src));
  memset(g_dest, 0, sizeof(g_dest));
}

#if defined(__x86_64__)
# define MEM_SUFFIX "(%%r15)"
#else
# define MEM_SUFFIX
#endif

/*
 * This test checks that non-temporal store instructions still have the
 * same effect after being rewritten to normal store instructions.
 */

int main(void) {
  /* Test movntdq. */
  reset_test_vars();
  asm("movdqa g_src " MEM_SUFFIX ", %%xmm0\n"
      "movntdq %%xmm0, g_dest " MEM_SUFFIX "\n" : : : "xmm0");
  ASSERT_EQ(memcmp(g_dest, g_src, 16), 0);

  /* Test prefetchnta.  This has no side effects that we can test for. */
  asm("prefetchnta g_dest " MEM_SUFFIX "\n" :);

  /* This compiles to prefetchnta on x86. */
  __builtin_prefetch(&g_dest, /* rw= */ 0, /* locality= */ 0);

  /* Test movntq. */
  reset_test_vars();
  asm("movq g_src" MEM_SUFFIX ", %%mm0\n"
      "movntq %%mm0, g_dest" MEM_SUFFIX "\n" : : : "mm0");
  ASSERT_EQ(memcmp(g_dest, g_src, 8), 0);

  /* Test movntps. */
  reset_test_vars();
  asm("movdqa g_src" MEM_SUFFIX ", %%xmm0\n"
      "movntps %%xmm0, g_dest" MEM_SUFFIX "\n" : : : "xmm0");
  ASSERT_EQ(memcmp(g_dest, g_src, 16), 0);

#if defined(__x86_64__)
  /* Test movnti, using 32-bit operand. */
  reset_test_vars();
  asm("mov g_src(%%r15), %%eax\n"
      "movnti %%eax, g_dest(%%r15)\n" : : : "eax");
  ASSERT_EQ(memcmp(g_dest, g_src, 4), 0);

  /* Test movnti, using 64-bit operand. */
  reset_test_vars();
  asm("mov g_src(%%r15), %%rax\n"
      "movnti %%rax, g_dest(%%r15)\n" : : : "rax");
  ASSERT_EQ(memcmp(g_dest, g_src, 8), 0);

  /* Test movnti, using a destination with a restricted register. */
  reset_test_vars();
  asm("mov g_src(%%r15), %%rax\n"
      ".p2align 5\n" /* Ensure following instructions are in the same bundle */
      "leal g_dest, %%ecx\n"
      "movnti %%rax, (%%r15, %%rcx)\n" : : : "rax", "rcx");
  ASSERT_EQ(memcmp(g_dest, g_src, 4), 0);

  /*
   * Test movnti, using a destination with RIP-relative addressing,
   * which is sensitive to the address of the instruction.
   */
  reset_test_vars();
  asm("mov g_src(%%r15), %%rax\n"
      "movnti %%rax, g_dest(%%rip)\n" : : : "rax");
  ASSERT_EQ(memcmp(g_dest, g_src, 8), 0);

  /* Test prefetchnta using RIP-relative addressing. */
  asm("prefetchnta g_dest(%rip)\n");
#endif

  return 0;
}
