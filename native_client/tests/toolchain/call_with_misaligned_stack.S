/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Call the argument with a stack pointer 4 bytes below its value on entry.
 * This will ensure that (assuming it's aligned on a 16-byte boundary on entry)
 * it will be misaligned on entry to the callee.
*/
    .text
    .p2align 5
    .globl   call_with_misaligned_stack
    .type    call_with_misaligned_stack, @function
call_with_misaligned_stack:
    movl     4(%esp), %ecx   /* call target */
    naclcall %ecx
    pop      %ecx
    nacljmp  %ecx
    .p2align 5
