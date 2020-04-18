/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

/*
 * Trampoline to transfer control from native client module to
 * sel_ldr's NaClSyscallSeg residing in the same address space
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

        /*
         * This is code, but it is not code that is actually part of the
         * program/library being linked.  Marking it as read-only data
         * instead ensures that nothing like linker code-rewriting will
         * be applied to this code.
         */
        NACL_RODATA

DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_seg_code):
        lcall   $0xcafe, $0xdeadbeef  /* orig-cs, thunk */
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_tramp_cseg_patch):
        hlt
DEFINE_GLOBAL_HIDDEN_LOCATION(NaCl_trampoline_seg_end):


DEFINE_GLOBAL_HIDDEN_LOCATION(NaClPcrelThunk):
        movl    $0xdeadbeef, %eax  /* known okay ds */
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClPcrelThunk_dseg_patch):
        mov     %ax, %ds  /* disable data sandbox for this thread */
        movl    $0xdeadbeef, %ecx  /* nacl_user */
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClPcrelThunk_globals_patch):
        /*
         * This is a regular PC-relative jump.  The displacement will be
         * patched in.  Using an explicit opcode with the largest possible
         * displacement here rather than "jmp" ensures that no too-clever
         * assembler or linker could ever produce a different opcode with a
         * smaller displacement.
         */
        .byte 0xe9, 0xff, 0xff, 0xff, 0x7f
DEFINE_GLOBAL_HIDDEN_LOCATION(NaClPcrelThunk_end):
