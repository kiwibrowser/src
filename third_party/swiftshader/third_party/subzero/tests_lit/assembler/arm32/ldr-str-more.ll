; More ldr/str examples (byte and half word).

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %lc2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %lc2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %lc2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %lc2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 | FileCheck %s --check-prefix=DIS


define internal i32 @LoadStoreI1(i32 %a, i32 %b) {
; ASM-LABEL:LoadStoreI1:
; DIS-LABEL:00000000 <LoadStoreI1>:
; IASM-LABEL:LoadStoreI1:

entry:
; ASM-NEXT:.LLoadStoreI1$entry:
; IASM-NEXT:.LLoadStoreI1$entry:

; ASM-NEXT:     sub     sp, sp, #32
; DIS-NEXT:   0:        e24dd020
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #28]
; ASM-NEXT:     # [sp, #28] = def.pseudo
; DIS-NEXT:   4:        e58d001c
; IASM-NEXT:    .byte 0x1c
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r1, [sp, #24]
; ASM-NEXT:     # [sp, #24] = def.pseudo
; DIS-NEXT:   8:        e58d1018
; IASM-NEXT:    .byte 0x18
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %b.arg_trunc = trunc i32 %b to i1

; ASM-NEXT:     ldr     r0, [sp, #24]
; DIS-NEXT:   c:        e59d0018
; IASM-NEXT:    .byte 0x18
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     and     r0, r0, #1
; DIS-NEXT:  10:        e2000001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     strb    r0, [sp, #20]
; ASM-NEXT:     # [sp, #20] = def.pseudo
; DIS-NEXT:  14:        e5cd0014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

  %a.arg_trunc = trunc i32 %a to i1
  %conv = zext i1 %a.arg_trunc to i32

; ASM-NEXT:     ldr     r0, [sp, #28]
; DIS-NEXT:  18:        e59d001c
; IASM-NEXT:    .byte 0x1c
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     and     r0, r0, #1
; DIS-NEXT:  1c:        e2000001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     strb    r0, [sp, #16]
; ASM-NEXT:     # [sp, #16] = def.pseudo
; DIS-NEXT:  20:        e5cd0010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

  %add = sext i1 %b.arg_trunc to i32

; ASM-NEXT:     ldrb    r0, [sp, #16]
; DIS-NEXT:  24:        e5dd0010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r0, [sp, #12]
; ASM-NEXT:     # [sp, #12] = def.pseudo
; DIS-NEXT:  28:        e58d000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %tobool4 = icmp ne i32 %conv, %add

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:  2c:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldrb    r1, [sp, #20]
; DIS-NEXT:  30:        e5dd1014
; IASM-NEXT:    .byte 0x14
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     tst     r1, #1
; DIS-NEXT:  34:        e3110001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x11
; IASM-NEXT:    .byte 0xe3

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

; ASM-NEXT:     str     r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:  40:        e58d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %tobool4.ret_ext = zext i1 %tobool4 to i32

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:  44:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldr     r1, [sp, #12]
; DIS-NEXT:  48:        e59d100c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r2, [sp, #8]
; DIS-NEXT:  4c:        e59d2008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     cmp     r1, r2
; DIS-NEXT:  50:        e1510002
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x51
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     movne   r0, #1
; DIS-NEXT:  54:        13a00001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0x13

; ASM-NEXT:     strb    r0, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:  58:        e5cd0004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

  ret i32 %tobool4.ret_ext

; ASM-NEXT:     ldrb    r0, [sp, #4]
; DIS-NEXT:  5c:        e5dd0004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r0, [sp]
; ASM-NEXT:     # [sp] = def.pseudo
; DIS-NEXT:  60:        e58d0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r0, [sp]
; DIS-NEXT:  64:        e59d0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     add     sp, sp, #32
; DIS-NEXT:  68:        e28dd020
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     bx      lr
; DIS-NEXT:  6c:        e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
}


define internal i32 @LoadStoreI16(i32 %a, i32 %b) {
; ASM-LABEL:LoadStoreI16:
; DIS-LABEL:00000070 <LoadStoreI16>:
; IASM-LABEL:LoadStoreI16:

entry:
; ASM-NEXT:.LLoadStoreI16$entry:
; IASM-NEXT:.LLoadStoreI16$entry:

; ASM-NEXT:     sub     sp, sp, #36
; DIS-NEXT:  70:        e24dd024
; IASM-NEXT:    .byte 0x24
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     str     r0, [sp, #32]
; ASM-NEXT:     # [sp, #32] = def.pseudo
; DIS-NEXT:  74:        e58d0020
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     str     r1, [sp, #28]
; ASM-NEXT:     # [sp, #28] = def.pseudo
; DIS-NEXT:  78:        e58d101c
; IASM-NEXT:    .byte 0x1c
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %b.arg_trunc = trunc i32 %b to i16

; ASM-NEXT:     ldr     r0, [sp, #28]
; DIS-NEXT:  7c:        e59d001c
; IASM-NEXT:    .byte 0x1c
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     strh    r0, [sp, #24]
; ASM-NEXT:     # [sp, #24] = def.pseudo
; DIS-NEXT:  80:        e1cd01b8
; IASM-NEXT:    .byte 0xb8
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe1

  %a.arg_trunc = trunc i32 %a to i16

; ASM-NEXT:     ldr     r0, [sp, #32]
; DIS-NEXT:  84:        e59d0020
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     strh    r0, [sp, #20]
; ASM-NEXT:     # [sp, #20] = def.pseudo
; DIS-NEXT:  88:        e1cd01b4
; IASM-NEXT:    .byte 0xb4
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe1

  %conv = zext i16 %a.arg_trunc to i32

; ASM-NEXT:     ldrh    r0, [sp, #20]
; DIS-NEXT:  8c:        e1dd01b4
; IASM-NEXT:    .byte 0xb4
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     uxth    r0, r0
; DIS-NEXT:  90:        e6ff0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xe6

; ASM-NEXT:     str     r0, [sp, #16]
; ASM-NEXT:     # [sp, #16] = def.pseudo
; DIS-NEXT:  94:        e58d0010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %conv1 = zext i16 %b.arg_trunc to i32

; ASM-NEXT:     ldrh    r0, [sp, #24]
; DIS-NEXT:  98:        e1dd01b8
; IASM-NEXT:    .byte 0xb8
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     uxth    r0, r0
; DIS-NEXT:  9c:        e6ff0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xe6

; ASM-NEXT:     str     r0, [sp, #12]
; ASM-NEXT:     # [sp, #12] = def.pseudo
; DIS-NEXT:  a0:        e58d000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %add = add i32 %conv1, %conv

; ASM-NEXT:     ldr     r0, [sp, #12]
; DIS-NEXT:  a4:        e59d000c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r1, [sp, #16]
; DIS-NEXT:  a8:        e59d1010
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     add     r0, r0, r1
; DIS-NEXT:  ac:        e0800001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe0

; ASM-NEXT:     str     r0, [sp, #8]
; ASM-NEXT:     # [sp, #8] = def.pseudo
; DIS-NEXT:  b0:        e58d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %conv2 = trunc i32 %add to i16

; ASM-NEXT:     ldr     r0, [sp, #8]
; DIS-NEXT:  b4:        e59d0008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     strh    r0, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:  b8:        e1cd00b4
; IASM-NEXT:    .byte 0xb4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe1

  %conv2.ret_ext = zext i16 %conv2 to i32

; ASM-NEXT:     ldrh    r0, [sp, #4]
; DIS-NEXT:  bc:        e1dd00b4
; IASM-NEXT:    .byte 0xb4
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     uxth    r0, r0
; DIS-NEXT:  c0:        e6ff0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xe6

; ASM-NEXT:     str     r0, [sp]
; ASM-NEXT:     # [sp] = def.pseudo
; DIS-NEXT:  c4:        e58d0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  ret i32 %conv2.ret_ext

; ASM-NEXT:     ldr     r0, [sp]
; DIS-NEXT:  c8:        e59d0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     add     sp, sp, #36
; DIS-NEXT:  cc:        e28dd024
; IASM-NEXT:    .byte 0x24
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     bx      lr
; DIS-NEXT:  d0:        e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

}
