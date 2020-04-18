; Show that we know how to encode CMN in the ARM integrated assembler.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

define internal i32 @testCmn(i32 %a) {
; ASM-LABEL:testCmn:
; DIS-LABEL:00000000 <testCmn>:
; IASM-LABEL:testCmn:

entry:
; ASM-NEXT:.LtestCmn$entry:
; IASM-NEXT:.LtestCmn$entry:

; ASM-NEXT:     sub     sp, sp, #12
; DIS-NEXT:   0:        e24dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:   4:        e58d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %cmp = icmp sgt i32 %a, -1

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:   8:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldr     r1, [sp, #8]
; DIS-NEXT:   c:        e59d1008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     cmn     r1, #1
; DIS-NEXT:  10:        e3710001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x71
; IASM-NEXT:    .byte 0xe3

  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
