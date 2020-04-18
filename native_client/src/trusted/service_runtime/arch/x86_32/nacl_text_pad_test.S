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
        movl 8(%esp), %eax  /* number to be printed */
        pushl %ebp
        pushl %ebx          /* callee saved */
        pushl %edi          /* callee saved */
        movl %esp, %ebp
        subl $0x10, %esp    /* buffer[0x10] */
        movl %ebp, %ebx     /* %ebx = &buffer[0x10] (off the end) */
        xorl %ecx, %ecx     /* %ecx = 0;  (byte count) */

        movl $10, %edi
do_digit_10:
        xorl %edx, %edx
        divl %edi           /* %edx:%eax div_mod 10 */
        addl $'0', %edx     /* *--%ebx = remainder + '0' */
        subl $1, %ebx
        movb %dl, (%ebx)
        addl $1, %ecx       /* ++byte_count */
        andl %eax, %eax
        jnz do_digit_10

        pushl %ecx
        pushl %ebx
        movl 0x10(%ebp), %eax
        pushl %eax
        call NACL_SYSCALL_ADDR(NACL_sys_write)
        addl $12, %esp

        movl %ebp, %esp
        popl %edi
        popl %ebx
        popl %ebp

        popl %ecx
        nacljmp %ecx

write_num_16:
        movl 8(%esp), %eax  /* number to be printed */
        pushl %ebp
        pushl %ebx
        movl %esp, %ebp
        subl $0x10, %esp
        movl %ebp, %ebx
        xorl %ecx, %ecx

do_digit_16:
        movl %eax, %edx
        andl $0xf, %edx
        addl $'0', %edx
        cmpl $(10 + '0'), %edx
        jl do_digit_not_upper
        addl $('a'-'0'-10), %edx
do_digit_not_upper:
        subl $1, %ebx
        movb %dl, (%ebx)
        addl $1, %ecx
        shrl $4, %eax
        jnz do_digit_16

        pushl %ecx
        pushl %ebx
        movl 0xc(%ebp), %eax
        pushl %eax
        call NACL_SYSCALL_ADDR(NACL_sys_write)
        addl $12, %esp

        movl %ebp, %esp
        popl %ebx
        popl %ebp

        popl %ecx
        nacljmp %ecx

        .p2align NACL_BLOCK_SHIFT
write_char:
        movl 4(%esp), %eax
        movl 8(%esp), %edx
        push %ebp
        movl %esp, %ebp
        subl $4, %esp
        lea -4(%ebp), %ecx
        movb %dl, (%ecx)
        pushl $1
        pushl %ecx
        pushl %eax
        call NACL_SYSCALL_ADDR(NACL_sys_write)
        addl $12, %esp
        movl %ebp, %esp
        popl %ebp
        popl %ecx
        nacljmp %ecx

        .p2align NACL_BLOCK_SHIFT
        .globl _start
_start:
        pushl $1234567
        pushl $1
        call write_num_10
        addl $8, %esp

        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp

        pushl $0xdeadbeef
        pushl $1
        call write_num_16
        addl $8, %esp

        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp

        pushl $end_of_text
        pushl $1
        call write_num_16
        addl $8, %esp
        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp

        pushl $0
        call NACL_SYSCALL_ADDR(NACL_sys_brk)
        addl $4, %esp

        xorl %edi, %edi  /* edi holds our eventual exit status */
        cmpl $EXPECTED_BREAK, %eax
        je brk_sane
        movl $1, %edi
brk_sane:

        pushl %eax
        pushl $1
        call write_num_16
        addl $8, %esp

        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp

        pushl $EXPECTED_BREAK
        pushl $1
        call write_num_16
        addl $8, %esp

        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp

#if defined(EXPECTED_RODATA)
        pushl $12
        pushl $ro_str
        pushl $1
        call NACL_SYSCALL_ADDR(NACL_sys_write)
        addl $12, %esp

        /* Truncate the address to page boundary for comparison. */
        leal ro_str, %ebx
        andl $-0x10000, %ebx

        pushl %ebx
        pushl $1
        call write_num_16
        addl $8, %esp

        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp

        cmpl $EXPECTED_RODATA, %ebx
        je ro_sane
        movl $2, %edi
ro_sane:

        pushl $EXPECTED_RODATA
        pushl $1
        call write_num_16
        addl $8, %esp

        pushl $'\n'
        pushl $1
        call write_char
        addl $8, %esp
#endif

        pushl %edi
        call NACL_SYSCALL_ADDR(NACL_sys_exit)
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
