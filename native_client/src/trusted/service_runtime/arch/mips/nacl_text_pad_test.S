/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/arch/mips/sel_ldr_mips.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#define STORE_MASK $t7
#define JUMP_MASK  $t6

        /*
         * Code to test various address layout boundary conditions.
         * This is a translation from the x86-{32,64} code, and is
         * not intended to be super efficent, just good enough to
         * print out the values that are designed, while maintaining
         * strict control over the address space layout.
         */

        .text
        .set noreorder
start_of_text:

        /*
         * write_num_16.  Write an unsigned value in hex.
         * a0 is descriptor number, a1 is unsigned value to write.
         */
        .p2align 4
write_num_16:
        addu  $sp, $sp, -40
        and   $sp, $sp, STORE_MASK
        sw    $ra, 36($sp)
        addu  $a3, $sp, 32

        move  $a2, $0
do_digit_16:
        addu  $a3, $a3, -1
        addu  $a2, $a2, 1
        and   $t0, $a1, 0xf

        addu  $t0, $t0, '0'
        addiu $t1, $t0, -57                /* 57 is ASCI value for '9'. */
        blez  $t1, do_digit_not_upper
        nop                                /* instructions in delay slot are
                                            * deliberately indented */

        addu  $t0, $t0, ('a'-'0'-10)
do_digit_not_upper:
        and   $a3, $a3, STORE_MASK
        sb    $t0, 0($a3)
        srl   $a1, $a1, 4

        bne   $a1, $0, do_digit_16
        nop
        move  $a1, $a3
        lui   $t9, %hi(NACL_SYSCALL_ADDR(NACL_sys_write))

        addiu $t9, $t9, %lo(NACL_SYSCALL_ADDR(NACL_sys_write))
        and   $t9, $t9, JUMP_MASK
        jalr  $t9
        nop

        lw    $ra, 36($sp)
        addu  $sp, $sp, 40
        and   $sp, $sp, STORE_MASK
        nop

        and   $ra,$ra,JUMP_MASK
        jr    $ra
        nop
        nop

        .p2align 4
        /*
         * a0 is descriptor number, a1 is character to write
         */
write_char:
        addu  $sp, $sp, -40
        and   $sp, $sp, STORE_MASK
        sw    $ra, 36($sp)
        nop

        and   $sp, $sp, STORE_MASK
        sb    $a1, 16($sp)
        addu  $a1, $sp, 16
        addu  $a2, $zero, 1

        lui   $t9, %hi(NACL_SYSCALL_ADDR(NACL_sys_write))
        addiu $t9, $t9, %lo(NACL_SYSCALL_ADDR(NACL_sys_write))
        nop
        nop

        nop
        and   $t9, $t9, JUMP_MASK
        jalr  $t9
        nop

        lw    $ra, 36($sp)
        addu  $sp, $sp, 40
        and   $sp, $sp, STORE_MASK
        nop

        and   $ra, $ra, JUMP_MASK
        jr    $ra
        nop
        nop

        .p2align 4
        .globl _start
_start:
        /* basic I/O test */
        addu  $sp, $sp, -16
        and   $sp, $sp, STORE_MASK
        addu  $a0, $zero, 1
        lui   $a1, 0xdead

        nop
        ori   $a1, $a1, 0xbeef            /* branch delay slot */
        jal   write_num_16
        nop

        addu  $a0, $zero, 1
        addu  $a1, $zero, '\n'
        jal   write_char
        nop

        lui   $a1, %hi(end_of_text)
        addiu $a1, $a1, %lo(end_of_text)
        jal   write_num_16
        addu  $a0, $zero, 1

        addu  $a0, $zero, 1
        addu  $a1, $zero, '\n'
        jal   write_char
        nop

        addu  $a0, $zero, 0
        lui   $t9, %hi(NACL_SYSCALL_ADDR(NACL_sys_brk))
        addiu $t9, $t9, %lo(NACL_SYSCALL_ADDR(NACL_sys_brk))
        nop

        nop
        and   $t9, $t9, JUMP_MASK
        jalr  $t9
        nop

        move  $s0, $v0                     /* save a copy of break addr */
        move  $a1, $v0
        jal   write_num_16
        addu  $a0, $zero, 1

        addu  $a0, $zero, 1
        addu  $a1, $zero, '\n'
        jal   write_char
        nop

        addu  $t5, $zero, 0                /* t5 holds eventual exit status */
        lui   $a1, %hi(EXPECTED_BREAK)
        addiu $a1, $a1, %lo(EXPECTED_BREAK)
        addu  $a0, $zero, 1

        sub   $t0, $a1, $s0
        movn  $t5, $a0, $t0
        jal   write_num_16
        nop

        addu  $a0, $zero, 1
        addu  $a1, $zero, '\n'
        jal   write_char
        nop

#if defined(EXPECTED_RODATA)
        addu  $a2, $zero, 12
        lui   $a1, %hi(ro_str)
        addiu $a1, $a1, %lo(ro_str)
        addu  $a0, $zero, 1

        lui   $t9, %hi(NACL_SYSCALL_ADDR(NACL_sys_write))
        addiu $t9, $t9, %lo(NACL_SYSCALL_ADDR(NACL_sys_write))
        nop
        nop

        nop
        and   $t9, $t9, JUMP_MASK
        jalr  $t9
        nop

        lui   $a1, %hi(ro_str)
        addiu $a1, $a1, %lo(ro_str)
        addu  $a0, $zero, 1
        nop

        nop
        nop
        jal   write_num_16
        nop

        addu  $a0, $zero, 1
        addu  $a1, $zero, '\n'
        jal   write_char
        nop

        lui   $a1, %hi(EXPECTED_RODATA)
        addiu $a1, $a1, %lo(EXPECTED_RODATA)
        lui   $a2, %hi(ro_str)
        addiu $a2, $a2, %lo(ro_str)

        addu  $a0, $zero, 1
        sub   $t0, $a1, $a2
        movn  $s0, $a0, $t0
        nop

        nop
        nop
        jal write_num_16
        nop

        addu  $a0, $zero, 1
        addu  $a1, $zero, '\n'
        jal   write_char
        nop
#endif

        lui   $t9, %hi(NACL_SYSCALL_ADDR(NACL_sys_exit))
        addiu $t9, $t9, %lo(NACL_SYSCALL_ADDR(NACL_sys_exit))
        addu  $a0, $zero, $s0
        nop

        nop
        and   $t9, $t9, JUMP_MASK
        jalr  $t9
        nop

#if defined(PNACL_AS)
/*
 * NOTE: unlike x86 the alignment directive on MIPS takes
 * the logarithm of the alignment
 */
#define POW2_BIGGER_THAN_DOT 14
        .align32 POW2_BIGGER_THAN_DOT, NACL_HALT_OPCODE
        .fill (TEXT_EXTEND - (1 << POW2_BIGGER_THAN_DOT))/4, 4, NACL_HALT_OPCODE
#else
        .fill (TEXT_EXTEND - (. - start_of_text))/4, 4, NACL_HALT_OPCODE
#endif

end_of_text:

#if defined(EXPECTED_RODATA)
        .section .rodata
ro_str: .ascii "Hello world\n"
#endif

