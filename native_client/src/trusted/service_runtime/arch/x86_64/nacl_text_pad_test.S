/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#if defined(PNACL_AS)
#define call naclcall
#endif
        .text
start_of_text:

        .p2align NACL_BLOCK_SHIFT
write_num_10:
        /* rdi has descriptor number, rsi has number to be printed */
        pushq %rbp
        movq %rsp, %rbp
        naclspadj $-0x20, %r15  /* buffer[0x20] */
        movl %ebp, %r8d         /* %r8 = &buffer[0x20] (off the end) */
        xorq %rcx, %rcx         /* %ecx = 0;  (byte count) */

        movq %rsi, %rax

        movq $10, %r9
do_digit_10:
        xorq %rdx, %rdx
        divq %r9                /* %rdx:%rax div_mod 10 */
        addq $'0', %rdx         /* *--%r8 = remainder + '0' */
        subl $1, %r8d
        movb %dl, (%r15, %r8)
        addq $1, %rcx           /* ++byte_count */
        andq %rax, %rax
        jnz do_digit_10

        movq %rcx, %rdx
        movq %r8, %rsi
        /* rdi still has descriptor number */
        movq $NACL_SYSCALL_ADDR(NACL_sys_write), %r10
        naclcall %r10d,%r15

        movq %rbp, %rsp
        popq %r11
        naclrestbp %r11d, %r15
        popq %r11
        nacljmp %r11d, %r15

        .p2align NACL_BLOCK_SHIFT
write_num_16:
        pushq %rbp
        movq %rsp, %rbp
        naclspadj $-0x20, %r15
        movl %ebp, %r8d
        xorq %rcx, %rcx
do_digit_16:
        movq %rsi, %rax
        andq $0xf, %rax
        addq $'0', %rax
        cmpq $(10 + '0'), %rax
        jl do_digit_not_upper
        addq $('a'-'0'-10), %rax
do_digit_not_upper:
        subl $1, %r8d
        movb %al, (%r15, %r8)
        addq $1, %rcx
        shrq $4, %rsi
        jnz do_digit_16

        movq %rcx, %rdx
        movq %r8, %rsi
        /* rdi still has descriptor number */
        movq $NACL_SYSCALL_ADDR(NACL_sys_write), %r10
        naclcall %r10d,%r15

        movq %rbp, %rsp
        popq %r11
        naclrestbp %r11d, %r15
        popq %r11
        nacljmp %r11d, %r15

        .p2align NACL_BLOCK_SHIFT
write_char:
        pushq %rbp
        movq %rsp, %rbp
        subl $0x10, %esp
        addq %r15, %rsp

        movl %esp, %r8d
        movb %sil, (%r15, %r8)
        movq $1, %rdx
        movl %esp, %esi
        /* %rdi retains value */
        movq $NACL_SYSCALL_ADDR(NACL_sys_write), %r10
        naclcall %r10d,%r15

        movq %rbp, %rsp
        popq %r11
        naclrestbp %r11d, %r15
        popq %r11
        nacljmp %r11d, %r15

        .p2align NACL_BLOCK_SHIFT
        .globl _start
_start:
        movq $123456789012345, %rsi
        movq $1, %rdi
        call write_num_10

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char

        movq $0xdeadbeefcafebabe, %rsi
        movq $1, %rdi
        call write_num_16

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char

        leaq end_of_text, %rsi
        movq $1, %rdi
        call write_num_16

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char

        movq $0, %rdi
        movq $NACL_SYSCALL_ADDR(NACL_sys_brk), %r10
        naclcall %r10d,%r15

        xorq %r12, %r12  /* r12 holds our eventual exit status */
        cmpl $EXPECTED_BREAK, %eax
        je brk_sane
        movq $1, %r12
brk_sane:

        movq %rax, %rsi
        movq $1, %rdi
        call write_num_16

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char

        movq $EXPECTED_BREAK, %rsi
        movq $1, %rdi
        call write_num_16

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char

#if defined(EXPECTED_RODATA)
        movq $12, %rdx
        movq $ro_str, %rsi
        movq $1, %rdi
        movq $NACL_SYSCALL_ADDR(NACL_sys_write), %r10
        naclcall %r10d,%r15

        /* Truncate the address to page boundary for comparison. */
        leaq ro_str, %rbx
        andq $-0x10000, %rbx

        movq %rbx, %rsi
        movq $1, %rdi
        call write_num_16

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char

        cmpq $EXPECTED_RODATA, %rbx
        je ro_sane
        movq $2, %r12
ro_sane:

        movq %rbx, %rsi
        movq $1, %rdi
        call write_num_16

        movq $'\n', %rsi
        movq $1, %rdi
        call write_char
#endif

        movq %r12, %rdi
        movq $NACL_SYSCALL_ADDR(NACL_sys_exit), %r10
        naclcall %r10d,%r15
      /*
         * Pad code section to TEXT_EXTEND with 0xf4.
         * This little dance is necessary because llvm mc does not support
         * arbitrary expressions in the first argument of .space".
         * nacl-as on the other hand gets confused by ".align"
         */
#if defined(PNACL_AS)
#define POW2_BIGGER_THAN_DOT 16384
        .align POW2_BIGGER_THAN_DOT, 0xf4
        .space TEXT_EXTEND - POW2_BIGGER_THAN_DOT, 0xf4
#else
        .skip TEXT_EXTEND - (. - start_of_text), 0xf4
#endif
end_of_text:

#if defined(EXPECTED_RODATA)
        .section .rodata
ro_str: .ascii "Hello world\n"
        /*      123456789012 */
#endif
