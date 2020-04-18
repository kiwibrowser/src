/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

.text
.globl test_tls_asm

test_tls_asm:
  push %rdi

  /*
   * This code should match the code in nacl_syscall_64.S
   */
// mov eax, OFFSET FLAT:tlsValue
// The following two lines generate this instruction:
//   ba XX XX XX XX    movl $tlsValue@SECREL, %edx
  .byte 0xba
  .secrel32 tlsValue

// mov ecx, DWORD PTR _tls_index
  mov _tls_index(%rip), %ecx

// mov rax, QWORD PTR gs:88
  movq %gs:0x58, %rax

// mov rax, QWORD PTR [rax+rcx*8]
  movq (%rax,%rcx,8), %rax

// mov rax, QWORD PTR [rdx+rax]
  movq (%rdx,%rax), %rax

  pop %rdi
  ret
