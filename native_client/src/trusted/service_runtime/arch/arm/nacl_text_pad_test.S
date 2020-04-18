/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "native_client/src/include/arm_sandbox.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/* the macros below are used with movw/movt and absolute values */
/* llvm-mc does not like the use of :lower16:/:upper16: with absolute values */
/* c.f.: http://llvm.org/bugs/show_bug.cgi?id=8721 */
#define lower_16(x) ((x) & 0xffff)
#define upper_16(x) (((x) >> 16) & 0xffff)


        /*
         * Code to test various address layout boundary conditions.
         * This is a translation from the x86-{32,64} code, and is
         * not intended to be super efficent, just good enough to
         * print out the values that are designed, while maintaining
         * strict control over the address space layout.
         */

        .text
start_of_text:
        /*
         * write_num_10 omitted.  __aeabi_udivmod is too long/complex
         * to replicate, we cannot link against glibc to get _udivsi3.o
         * and pick up transitive dependencies.  Plus, we don't actually
         * use decimal output (yet).
         *
         * One alternative would be to link using:
         *
         * ${LD} --native-client -e _start -Ttext 0x20000 -o ${TARGET} \
         *  ${SOURCES} ${GLIBC} ${TEXT_EXTEND}
         *
         * i.e., to load the text pad as a separate .o *after* external
         * references are resolved from glibc, with link-time
         * expansion/computation of the (TEXT_EXTEND - (. - start_of_text))/4
         * fill expression (is that possible?).
         */

        /*
         * write_num_16.  Write an unsigned value in hex.
         * r0 is descriptor number, r1 is unsigned value to write.
         */
        .p2align 4
write_num_16:
        push {r4, lr}
        mov r3, sp
        /* r12 is caller-saved: use it as a temporary register */
        sub r12, sp, #0x10
        bic sp, r12, #0xc0000000

        mov r2, #0
do_digit_16:
        sub r3, r3, #1
        add r2, r2, #1
        and r4, r1, #0xf

        add r4, r4, #'0'
        cmp r4, #'9'
        ble do_digit_not_upper
        add r4, r4, #('a'-'0'-10)

do_digit_not_upper:
        bic r3, r3, #0xc0000000
        strb r4, [r3]
        lsrs r1, r1, #4
        bne do_digit_16

        mov r1, r3
        movw r3, lower_16(NACL_SYSCALL_ADDR(NACL_sys_write))
        movt r3, upper_16(NACL_SYSCALL_ADDR(NACL_sys_write))
        nop

        nop
        nop
        bic r3, r3, #0xc000000f
        blx r3

        /* r12 is caller-saved: use it as a temporary register */
        add r12, sp, #0x10
        bic sp, r12, #0xc0000000
        pop {r4, lr}
        nop

        bic lr, lr, #0xc000000f
        bx lr

        .p2align 4
        /*
         * r0 is descriptor number, r1 is character to write
         */
write_char:
        push {lr}
        /* r12 is caller-saved: use it as a temporary register */
        sub r12, sp, #0x10
        bic sp, r12, #0xc0000000
        strb r1, [sp]

        mov r1, sp
        mov r2, #1
        movw r3, lower_16(NACL_SYSCALL_ADDR(NACL_sys_write))
        movt r3, upper_16(NACL_SYSCALL_ADDR(NACL_sys_write))

        nop
        nop
        bic r3, r3, #0xc000000f
        blx r3

        /* r12 is caller-saved: use it as a temporary register */
        add r12, sp, #0x10
        bic sp, r12, #0xc0000000
        pop {lr}
        nop

        bic lr, lr, #0xc000000f
        bx lr

        .p2align 4
        .globl _start
_start:
        /* basic I/O test */
        mov r0, #1
        movw r1, #0xbeef
        movt r1, #0xdead
        bl write_num_16

        mov r0, #1
        mov r1, #'\n'
        nop
        bl write_char

        movw r1, :lower16:(end_of_text)
        movt r1, :upper16:(end_of_text)
        mov r0, #1
        bl write_num_16

        mov r0, #1
        mov r1, #'\n'
        nop
        bl write_char

        mov r0, #0
        movw r3, lower_16(NACL_SYSCALL_ADDR(NACL_sys_brk))
        movt r3, upper_16(NACL_SYSCALL_ADDR(NACL_sys_brk))
        nop

        nop
        nop
        bic r3, r3, #0xc000000f
        blx r3

        mov r4, r0  /* save a copy of break addr */
        mov r1, r0
        mov r0, #1
        bl write_num_16

        mov r0, #1
        mov r1, #'\n'
        nop
        bl write_char

        mov r5, #0  /* r5 holds eventual exit status */
        movw r1, lower_16(EXPECTED_BREAK)
        movt r1, upper_16(EXPECTED_BREAK)
        cmp r1, r4

        movne r5, #1
        mov r0, #1  /* r1 still has EXPECTED_BREAK */
        nop
        bl write_num_16

        mov r1, #'\n'
        mov r0, #1
        nop
        bl write_char

#if defined(EXPECTED_RODATA)
        mov r2, #12
        movw r1, :lower16:(ro_str)
        movt r1, :upper16:(ro_str)
        mov r0, #1

        movw r3, lower_16(NACL_SYSCALL_ADDR(NACL_sys_write))
        movt r3, upper_16(NACL_SYSCALL_ADDR(NACL_sys_write))
        bic r3, r3, #0xc000000f
        blx r3

        /* Truncate the address to page boundary for comparison. */
        movw r4, #0
        movt r4, :upper16:(ro_str)
        nop
        nop

        mov r0, #1
        mov r1, r4
        nop
        bl write_num_16

        mov r1, #'\n'
        mov r0, #1
        nop
        bl write_char

        movw r1, lower_16(EXPECTED_RODATA)
        movt r1, upper_16(EXPECTED_RODATA)
        nop
        nop

        cmp r1, r4
        movne r5, #2
        mov r0, #1  /* r1 still has EXPECTED_RODATA */
        bl write_num_16

        mov r1, #'\n'
        mov r0, #2
        nop
        bl write_char
#endif

        movw r3, lower_16(NACL_SYSCALL_ADDR(NACL_sys_exit))
        movt r3, upper_16(NACL_SYSCALL_ADDR(NACL_sys_exit))
        mov r0, r5
        nop

        nop
        nop
        bic r3, r3, #0xc000000f
        blx r3
#if defined(PNACL_AS)
/* NOTE: unlike x86 the alignment directive on ARM takes
   the logarithm of the alignment */
#define POW2_BIGGER_THAN_DOT 14
        .align32 POW2_BIGGER_THAN_DOT, NACL_INSTR_ARM_HALT_FILL
        .fill (TEXT_EXTEND - (1 << POW2_BIGGER_THAN_DOT))/4, 4, \
              NACL_INSTR_ARM_HALT_FILL
#else
        .fill (TEXT_EXTEND - (. - start_of_text))/4, 4, \
              NACL_INSTR_ARM_HALT_FILL
#endif

end_of_text:

#if defined(EXPECTED_RODATA)
        .section .rodata
ro_str: .ascii "Hello world\n"
        /*      123456789012 */
#endif
