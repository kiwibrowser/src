/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This tests that the compiler handles __builtin_prefetch.
 * All we're testing is that the compiler doesn't fall over
 * and that it doesn't produce code that goes bad.  We don't
 * try to test that it actually emits prefetch instructions.
 * (As of 2014-6-18, arm-nacl-gcc and x86_64-nacl-gcc -m64 do
 * but x86_64-nacl-gcc -m32 and all pnacl compilers do not.)
 * This is a regression test for:
 *      https://code.google.com/p/nativeclient/issues/detail?id=3582
 */

static void DoPrefetch(char *p) {
  __builtin_prefetch(p);
  __builtin_prefetch(p + 17);
}

static char g_buf[32];

int main(void) {
  char l_buf[32];

  DoPrefetch(l_buf);  /* Stack case: shouldn't require sandboxing.  */
  DoPrefetch(g_buf);  /* Heap case: should require sandboxing.  */

  return 0;
}
