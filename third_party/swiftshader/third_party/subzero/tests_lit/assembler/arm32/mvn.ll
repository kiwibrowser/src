; Tests MVN instruction.

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

define internal void @mvnEx(i32 %a, i32 %b) {
; ASM-LABEL:mvnEx:
; DIS-LABEL:00000000 <mvnEx>:
; IASM-LABEL:mvnEx:

entry:
; ASM-NEXT:.LmvnEx$entry:
; IASM-NEXT:.LmvnEx$entry:

; ASM-NEXT:     sub     sp, sp, #24
; DIS-NEXT:   0:        e24dd018
; IASM-NEXT:    .byte 0x18
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #20]
; ASM-NEXT:     # [sp, #20] = def.pseudo
; DIS-NEXT:   4:        e58d0014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r1, [sp, #16]
; ASM-NEXT:     # [sp, #16] = def.pseudo
; DIS-NEXT:   8:        e58d1010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %b.arg_trunc = trunc i32 %b to i1

; ASM-NEXT:     ldr     r0, [sp, #16]
; DIS-NEXT:   c:        e59d0010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     and     r0, r0, #1
; DIS-NEXT:  10:        e2000001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     strb    r0, [sp, #12]
; ASM-NEXT:     # [sp, #12] = def.pseudo
; DIS-NEXT:  14:        e5cd000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

  %a.arg_trunc = trunc i32 %a to i1

; ASM-NEXT:     ldr     r0, [sp, #20]
; DIS-NEXT:  18:        e59d0014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     and     r0, r0, #1
; DIS-NEXT:  1c:        e2000001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     strb    r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:  20:        e5cd0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

  %conv = zext i1 %a.arg_trunc to i32

; ASM-NEXT:     ldrb    r0, [sp, #8]
; DIS-NEXT:  24:        e5dd0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r0, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:  28:        e58d0004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %ignore = sext i1 %b.arg_trunc to i32

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:  2c:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldrb    r1, [sp, #12]
; DIS-NEXT:  30:        e5dd100c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     tst     r1, #1
; DIS-NEXT:  34:        e3110001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x11
; IASM-NEXT:    .byte 0xe3

; ********* Use of MVN ********
; ASM-NEXT:     mvn     r1, #0
; DIS-NEXT:  38:        e3e01000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xe0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     movne   r0, r1
; DIS-NEXT:  3c:        11a00001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0x11

  ret void
}
