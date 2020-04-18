; Test that we handle cmp (register) and cmp (immediate).

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

define internal i32 @cmpEqI8(i32 %a, i32 %b) {
; ASM-LABEL:cmpEqI8:
; DIS-LABEL:00000000 <cmpEqI8>:
; IASM-LABEL:cmpEqI8:


entry:
; ASM-NEXT:.LcmpEqI8$entry:
; IASM-NEXT:.LcmpEqI8$entry:

  %b.arg_trunc = trunc i32 %b to i8
  %a.arg_trunc = trunc i32 %a to i8

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

; ASM-NEXT:     ldr     r0, [sp, #16]
; DIS-NEXT:   c:        e59d0010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     strb    r0, [sp, #12]
; ASM-NEXT:     # [sp, #12] = def.pseudo
; DIS-NEXT:  10:       e5cd000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r0, [sp, #20]
; DIS-NEXT:  14:        e59d0014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     strb    r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:  18:        e5cd0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:  1c:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldrb    r1, [sp, #8]
; DIS-NEXT:  20:        e5dd1008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     lsl     r1, r1, #24
; DIS-NEXT:  24:        e1a01c01
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x1c
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     ldrb    r2, [sp, #12]
; DIS-NEXT:  28:        e5dd200c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ******** CMP instruction test **************

  %cmp = icmp eq i8 %a.arg_trunc, %b.arg_trunc

; ASM-NEXT:        cmp     r1, r2, lsl #24
; DIS-NEXT:  2c:        e1510c02
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x51
; IASM-NEXT:    .byte 0xe1

  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}

define internal i32 @cmpEqI32(i32 %a, i32 %b) {
; ASM-LABEL:cmpEqI32:
; DIS-LABEL:00000050 <cmpEqI32>:
; IASM-LABEL:cmpEqI32:

entry:
; ASM-NEXT:.LcmpEqI32$entry:
; IASM-NEXT:.LcmpEqI32$entry:

; ASM-NEXT:     sub     sp, sp, #16
; DIS-NEXT:  50:        e24dd010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #12]
; ASM-NEXT:     # [sp, #12] = def.pseudo
; DIS-NEXT:  54:        e58d000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r1, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:  58:        e58d1008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:  5c:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldr     r1, [sp, #12]
; DIS-NEXT:  60:        e59d100c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r2, [sp, #8]
; DIS-NEXT:  64:        e59d2008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ******** CMP instruction test **************

  %cmp = icmp eq i32 %a, %b

; ASM-NEXT:     cmp     r1, r2
; DIS-NEXT:  68:        e1510002
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x51
; IASM-NEXT:    .byte 0xe1

  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
