/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

void unfixed_code(void);

#if defined(__i386__) || defined(__x86_64__)
__asm__("unfixed_code: ret\n");
#elif defined(__mips__)
/*
 * We need to mark unfixed_code as global, otherwise linker can not resolve pic
 * CALL16 relocation against local symbol unfixed_code.
 */
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".global unfixed_code\n"
        "unfixed_code: jr $ra\n"
                      "nop\n"
        ".popsection\n");
#else
# error "Unsupported architecture"
#endif

/* Test a large number of validator errors.  This is a regression test
   for a bug in which the validator would only stub out the first 100
   bad instructions.
   See http://code.google.com/p/nativeclient/issues/detail?id=1194.
   The assembler does not provide an obvious repeat/loop construct, so
   we use ".fill" with a numeric value to generate an instruction many
   times. */
#if defined(__i386__)
__asm__(".p2align 5\n"
        /* "cd 80" disassembles to "int $0x80". */
        ".fill 1000, 2, 0x80cd\n"
        ".p2align 5\n");
#elif defined(__x86_64__)
__asm__(".p2align 5\n"
        /* "0f 05" disassembles to "syscall". */
        ".fill 1000, 2, 0x050f\n"
        ".p2align 5\n");
#elif defined(__mips__)
__asm__(".pushsection .text, \"ax\", @progbits\n"
        ".p2align 4\n"
        /* "0xc" disassembles to "syscall". */
        ".fill 1000, 4, 0x0000000c\n"
        ".p2align 4\n"
        ".popsection\n");
#else
# error "Unsupported architecture"
#endif

int main(void) {
  printf("Before non-validating instruction\n");
  fflush(stdout);
  unfixed_code();
  printf("After non-validating instruction\n");
  return 0;
}
