/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

int main(void) {
#if defined(__i386__) || defined(__x86_64__)
  uint8_t code[] __attribute__((aligned(32))) = { 0xc3 /* ret */ };
#elif defined(__arm__)
  uint32_t code[] __attribute__((aligned(32))) = { 0xe12fff1e /* BX LR */ };
#elif defined(__mips__)
  uint32_t code[] __attribute__((aligned(32))) = { 0x03e00008, /* JR RA */
                                                   0           /* NOP   */ };
#else
# error Unknown architecture
#endif

  void (*func)(void);

  /* Double cast required to stop gcc complaining. */
  func = (void (*)(void)) (uintptr_t) code;

  fprintf(stderr, "** intended_exit_status=untrusted_sigsegv_or_equivalent\n");
  /* This should fault because the data segment should not be executable. */
  func();
  fprintf(stderr, "We're still running. This is bad.\n");
  return 1;
}
