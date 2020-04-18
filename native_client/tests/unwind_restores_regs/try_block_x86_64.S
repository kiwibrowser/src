/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* Save all callee-saved registers to a global struct. */
#define SAVE_REGS(global_var) \
        movq %r14, global_var + 8*0(%rip); \
        movq %r13, global_var + 8*1(%rip); \
        movq %r12, global_var + 8*2(%rip); \
        movq %rbx, global_var + 8*3(%rip)


        .global __gxx_personality_v0

        .text

        /*
         * This function saves registers, clobbers the same
         * registers, and then throws an exception.
         */
        .align 32
clobber_regs_and_throw:
        .cfi_startproc
        pushq %rbp
        .cfi_def_cfa_offset 16
        movq %rsp, %rbp
        .cfi_offset 6, -16
        .cfi_def_cfa_register 6
        /* Save registers so that we can clobber them. */
        pushq %r14
        .cfi_offset 14, -24
        pushq %r13
        .cfi_offset 13, -32
        pushq %r12
        .cfi_offset 12, -40
        pushq %rbx
        .cfi_offset 3, -48
        /*
         * Clobber the callee-saved registers in order to test that
         * this is undone by libgcc_eh's stack unwinding.
         */
        mov $0x9999, %rbx
        mov $0x9999, %r12
        mov $0x9999, %r13
        mov $0x9999, %r14
        call throw_some_exception
        /* No return expected. */
        hlt
        .cfi_endproc


        /*
         * This function sets registers to known values, throws an
         * exception, and then checks that the registers are restored
         * when handling the exception.
         */
        .align 32
        .global try_block_test
try_block_test:
.L_func_start:
        .cfi_startproc
        .cfi_personality 0x3,__gxx_personality_v0
        .cfi_lsda 0x3,.L_lsda_start
        naclsspq $8, %r15
        .cfi_def_cfa_offset 16
.L_catch_start:
        /* Set callee-saved registers to known values. */
        mov $0x1234009b5678008b, %rbx
        mov $0x1234009c5678008c, %r12
        mov $0x1234009d5678008d, %r13
        mov $0x1234009e5678008e, %r14
        SAVE_REGS(g_expected_regs)
        call clobber_regs_and_throw
        hlt
.L_catch_end:
        /* Bundle-align because landing pad is indirect-jumped to. */
        .align 32
.L_landingpad:
        /* Check that callee-saved registers were restored correctly. */
        SAVE_REGS(g_actual_regs)
        call check_register_state
        hlt
        .cfi_endproc


        /* LSDA for the function try_block_test(). */
        .section .gcc_except_table,"a",@progbits
.L_lsda_start:
        .byte   0xff  /* DW_EH_PE_omit:  encoding of landing pad base */
        .byte   0xff  /* DW_EH_PE_omit:  encoding of type table entries */
        .byte   0x1   /* DW_EH_PE_uleb128:  encoding of call site table */
        /* Size of call site table */
        .uleb128 .L_call_sites_end - .L_call_sites_rel
        /* Call site table (with 1 entry): */
.L_call_sites_rel:
        .uleb128 .L_catch_start - .L_func_start  /* Catch range start */
        .uleb128 .L_catch_end - .L_catch_start   /* Catch range length */
        .uleb128 .L_landingpad - .L_func_start   /* Landing pad start */
        .uleb128 0  /* Action to take: cleanup only */
.L_call_sites_end:
