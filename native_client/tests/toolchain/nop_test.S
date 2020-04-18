/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

        .text

/*
 * This test attempts to get both the assembler and the linker to generate
 * all the different kinds of nop instructions they will generate when
 * doing alignment padding in the code segment.  Running the output through
 * the validator ensures that it accepts all these nop forms.
 */

.macro fill_before length
        /*
         * Fill space at the start of a bundle leaving exactly |length|
         * bytes before the end of the bundle.
         */
        .fill ((1 << NACL_BLOCK_SHIFT) - \length) / NACL_HALT_LEN, \
              NACL_HALT_LEN, NACL_HALT_OPCODE
.endm

.macro try length
        /*
         * Create the circumstance where exactly |length| bytes must be
         * filled to perform '.p2align NACL_BLOCK_SHIFT', and then let the
         * assembler fill it with its choice of nops.
         */
        .p2align NACL_BLOCK_SHIFT
test_as_nop_\length\():
        fill_before \length
        .p2align NACL_BLOCK_SHIFT
        nop

         /*
          * Now create the same circumstance in the linker when it stitches
          * this section to the next one (which has sh_addralign=32).
          */
        .pushsection .text.nop.\length, "ax", %progbits
        .p2align NACL_BLOCK_SHIFT
test_ld_nop_\length\():
        fill_before \length
        .popsection
.endm

#if NACL_BLOCK_SHIFT == 5 && NACL_HALT_LEN == 1
try 1
try 2
try 3
try 4
try 5
try 6
try 7
try 8
try 9
try 10
try 11
try 12
try 13
try 14
try 15
try 16
try 17
try 18
try 19
try 20
try 21
try 22
try 23
try 24
try 25
try 26
try 27
try 29
try 30
try 31
#elif NACL_BLOCK_SHIFT == 4 && NACL_HALT_LEN == 4
try 4
try 8
try 12
#else
# error "architecture not handled by this test"
#endif

        /*
         * This section follows .text.nop.31 and creates (by virtue of its
         * alignment) the need for the linker to generate the last nop
         * sequence.
         */
        .pushsection .text.nop.end, "ax", %progbits
        .p2align NACL_BLOCK_SHIFT
        nop
        .popsection
