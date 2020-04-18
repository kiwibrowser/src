; Show that we know how to translate mls.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 --mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --mattr=hwdiv-arm | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 --mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --mattr=hwdiv-arm | FileCheck %s --check-prefix=DIS

define internal i32 @testMls(i32 %a, i32 %b) {
; ASM-LABEL: testMls:
; DIS-LABEL: 00000000 <testMls>:
; IASM-LABEL: testMls:

entry:
; ASM-NEXT: .LtestMls$entry:
; IASM-NEXT: .LtestMls$entry:

; ASM-NEXT:     sub     sp, sp, #12
; DIS-NEXT:    0:       e24dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:    4:       e58d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r1, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:    8:       e58d1004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %rem = srem i32 %a, %b

; ASM-NEXT:     ldr     r0, [sp, #8]
; DIS-NEXT:    c:       e59d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r1, [sp, #4]
; DIS-NEXT:   10:       e59d1004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:    tst     r1, r1
; DIS-NEXT:   14:       e1110001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x11
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     bne     .LtestMls$local$__0
; DIS-NEXT:   18:       1a000000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x1a

; ASM-NEXT:     .long 0xe7fedef0
; DIS-NEXT:   1c:       e7fedef0
; IASM-NEXT:    .byte 0xf0
; IASM-NEXT:    .byte 0xde
; IASM-NEXT:    .byte 0xfe
; IASM-NEXT:    .byte 0xe7

; ASM-NEXT: .LtestMls$local$__0:
; IASM-NEXT: .LtestMls$local$__0:

; ASM-NEXT:     ldr     r1, [sp, #4]
; DIS-NEXT:   20:       e59d1004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     sdiv    r2, r0, r1
; DIS-NEXT:   24:       e712f110
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xf1
; IASM-NEXT:    .byte 0x12
; IASM-NEXT:    .byte 0xe7

; ASM-NEXT:     mls     r0, r2, r1, r0
; DIS-NEXT:   28:       e0600192
; IASM-NEXT:    .byte 0x92
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x60
; IASM-NEXT:    .byte 0xe0

  ret i32 %rem
}
