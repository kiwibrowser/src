/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Rounding mode control field is bits [11:10] in the x87
 * control word, and bits [14:13] in the SSE control word.
 * This function takes in %ecx the two-bit value pre-shifted to
 * bits [11:10] and sets both control words at the appropriate bits.
 *
 * The bit patterns for X86 are:
 * 0b00 Round to Nearest (RN) mode
 * 0b10 Round towards Plus Infinity (RP) mode
 * 0b01 Round towards Minus Infinity (RM) mode
 * 0b11 Round towards Zero (RZ) mode
 */
        .p2align 5
set_rounding_control_field:
        /* First set the 87 control word */
#if defined(__x86_64__)
        fnstcw -8(%rsp) # Use red zone.
        movl -8(%rsp), %eax
#else
        sub $12, %esp
        fnstcw 0(%esp)
        movl 0(%esp), %eax
#endif
        andl $0xfffff3ff, %eax
        orl %ecx, %eax
#if defined(__x86_64__)
        movl %eax, -8(%rsp)
        fldcw -8(%rsp)
#else
        movl %eax, 0(%esp)
        fldcw 0(%esp)
#endif
        /* Then set the SSE control word w/ same pattern, but shifted. */
#if defined(__x86_64__)
        stmxcsr -8(%rsp)
        movl -8(%rsp), %eax
#else
        stmxcsr 0(%esp)
        movl 0(%esp), %eax
#endif
        andl $0xffff9fff, %eax
        shl $3, %ecx
        orl %ecx, %eax
#if defined(__x86_64__)
        movl %eax, -8(%rsp)
        ldmxcsr -8(%rsp)
#else
        movl %eax, 0(%esp)
        ldmxcsr 0(%esp)
        add $12, %esp
#endif
        naclret

        .p2align 5
        .global set_round_toward_nearest
set_round_toward_nearest:
        /* Use a fast-cc + tail call to avoid stack manipulation/alignment */
        movl $0, %ecx
        jmp set_rounding_control_field

        .p2align 5
        .global set_round_toward_plus_infinity
set_round_toward_plus_infinity:
        movl $0x800, %ecx
        jmp set_rounding_control_field

        .p2align 5
        .global set_round_toward_minus_infinity
set_round_toward_minus_infinity:
        movl $0x400, %ecx
        jmp set_rounding_control_field

        .p2align 5
        .global set_round_toward_zero
set_round_toward_zero:
        movl $0xc00, %ecx
        jmp set_rounding_control_field
