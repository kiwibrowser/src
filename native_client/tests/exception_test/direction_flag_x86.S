/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

        /*
         * get_x86_direction_flag() returns whether the x86 direction
         * flag is set.  The normal way of reading the flag is to use
         * "pushf", but this instruction is not allowed under NaCl.
         * So instead, we test the flag empirically by checking how a
         * string instruction changes its operand register.
         */

#if defined(__x86_64__)

        .global get_x86_direction_flag
get_x86_direction_flag:
        naclssp $8, %r15

        mov %esp, %esi
        mov %esp, %edi
        movsb %nacl:(%rsi), %nacl:(%rdi), %r15

        /* Clear %eax */
        xor %eax, %eax
        /* This sets the bottom byte of %eax if %edi < %esp (unsigned) */
        cmp %esp, %edi
        setb %al

        naclasp $8, %r15
        naclret

#elif defined(__i386__)

        .global get_x86_direction_flag
get_x86_direction_flag:
        sub $4, %esp

        mov %esp, %esi
        mov %esp, %edi
        movsb %ds:(%esi), %es:(%edi)

        /* Clear %eax */
        xor %eax, %eax
        /* This sets the bottom byte of %eax if %edi < %esp (unsigned) */
        cmp %esp, %edi
        setb %al

        add $4, %esp
#if defined(__native_client_nonsfi__)
        ret
#else
        naclret
#endif

#else
# error Unknown platform
#endif
