/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "templates.h"

#define SANDBOX_MASK NACL_CONTROL_FLOW_MASK

        .global template_func
        .global template_func_end
        .p2align 4
template_func:
        /* We start this function with few nops to make
         * test_replacing_code_unaligned() work, as it replace
         * instructions inside bundle.
         */
        nop
        nop
        mov r0, #MARKER_OLD & 0xff
        orr r0, #MARKER_OLD & 0xff00
        bic lr, lr, #SANDBOX_MASK
        bx  lr
        and r1, r1, #42
        bl  template_func
template_func_end:
        .global template_func_replacement
        .global template_func_replacement_end
        .p2align 4
template_func_replacement:
        /* See comment above. */
        nop
        nop
        mov r0, #MARKER_NEW & 0xff
        orr r0, #MARKER_NEW & 0xff00
        bic lr, lr, #SANDBOX_MASK
        bx  lr
        and r1, r1, #42
        bl  template_func_replacement
template_func_replacement_end:

        .global template_func_nonreplacement
        .global template_func_nonreplacement_end
        .p2align 4
template_func_nonreplacement:
        mov r0, #MARKER_OLD & 0xff
        orr r0, #MARKER_OLD & 0xff00
        mov r1, #MARKER_OLD & 0xff
        orr r1, #MARKER_OLD & 0xff00
        bic lr, lr, #SANDBOX_MASK
        bx  lr
template_func_nonreplacement_end:

        .global hlts
        .global hlts_end
        .p2align 4
hlts:
        .word NACL_INSTR_ARM_HALT_FILL
        .word NACL_INSTR_ARM_HALT_FILL
        .word NACL_INSTR_ARM_HALT_FILL
        .word NACL_INSTR_ARM_HALT_FILL
hlts_end:


        .global branch_forwards
        .global branch_forwards_end
        .global branch_backwards
        .global branch_backwards_end
        .p2align 4
branch_forwards:
        b branch_backwards
        .p2align 4
branch_forwards_end:
branch_backwards:
        b branch_forwards
        .p2align 4
branch_backwards_end:

       /*
        * We include disallowed code below, so this must go into the
        * data segment.
        */
        .data

        .global invalid_code
        .global invalid_code_end
        .p2align 4
invalid_code:
        bx lr
invalid_code_end:

        .global template_func_misaligned_replacement
        .global template_func_misaligned_replacement_end
        .p2align 4
template_func_misaligned_replacement:
        nop
        mov r0, #MARKER_NEW & 0xff
        orr r0, #MARKER_NEW & 0xff00
        bic lr, lr, #SANDBOX_MASK
        bx  lr
        nop
        and r1, r1, #42
        bl  template_func_misaligned_replacement

template_func_misaligned_replacement_end:

        .global template_func_illegal_register_replacement
        .global template_func_illegal_register_replacement_end
        .p2align 4
template_func_illegal_register_replacement:
        nop
        nop
        mov r0, #MARKER_OLD & 0xff
        orr r0, #MARKER_OLD & 0xff00
        bic r2, r2, #SANDBOX_MASK
        bx  lr
        and r1, r1, #42
        bl  template_func_illegal_register_replacement
template_func_illegal_register_replacement_end:

        .global template_func_illegal_guard_replacement
        .global template_func_illegal_guard_replacement_end
        .p2align 4
template_func_illegal_guard_replacement:
        nop
        nop
        mov r0, #MARKER_OLD & 0xff
        orr r0, #MARKER_OLD & 0xff00
        bic lr, lr, #SANDBOX_MASK
        bx  r2
        and r1, r1, #42
        bl  template_func_illegal_guard_replacement
template_func_illegal_guard_replacement_end:

        .global template_func_illegal_call_target
        .global template_func_illegal_call_target_end
        .p2align 4
template_func_illegal_call_target:
        nop
        nop
        mov r0, #MARKER_NEW & 0xff
        orr r0, #MARKER_NEW & 0xff00
        bic lr, lr, #SANDBOX_MASK
0:      bx  lr
        and r1, r1, #42
        bl  0b
template_func_illegal_call_target_end:

        .global template_func_illegal_constant_replacement
        .global template_func_illegal_constant_replacement_end
        .p2align 4
template_func_illegal_constant_replacement:
        nop
        nop
        mov r0, #MARKER_NEW & 0xff
        orr r0, #MARKER_NEW & 0xff00
        bic lr, lr, #SANDBOX_MASK
        bx  lr
        and r1, r1, #52  /* can't change constant in and */
        bl  template_func_illegal_constant_replacement
template_func_illegal_constant_replacement_end:

        .global template_func_external_jump_target
        .global template_func_external_jump_target_end
        .p2align 4
template_func_external_jump_target:
        mov r0, #MARKER_OLD & 0xff
        orr r0, #MARKER_OLD & 0xff00
        b external_jump_return
        bl template_func_external_jump_target
        mov r0, #MARKER_STABLE & 0xff
        orr r0, #MARKER_STABLE & 0xff00
        nop
        nop
external_jump_return:
        bic lr, lr, #SANDBOX_MASK
        bx  lr
template_func_external_jump_target_end:

        .global template_func_external_jump_target_replace
        .global template_func_external_jump_target_replace_end
        .p2align 4
template_func_external_jump_target_replace:
        mov r0, #MARKER_NEW & 0xff
        orr r0, #MARKER_NEW & 0xff00
        b external_jump_replace_return
        bl template_func_external_jump_target_replace
        mov r0, #MARKER_STABLE & 0xff
        orr r0, #MARKER_STABLE & 0xff00
        nop
        nop
external_jump_replace_return:
        bic lr, lr, #SANDBOX_MASK
        bx  lr
template_func_external_jump_target_replace_end:
