; Test that we correctly fix multiple forward branches.

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

; REQUIRES: allow_dump

define internal void @mult_fwd_branches(i32 %a, i32 %b) {
; ASM-LABEL:mult_fwd_branches:
; DIS-LABEL:00000000 <mult_fwd_branches>:
; IASM-LABEL:mult_fwd_branches:

; ASM-LABEL:.Lmult_fwd_branches$__0:
; IASM-LABEL:.Lmult_fwd_branches$__0:

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

; ASM-NEXT:     str     r1, [sp, #4]
; ASM-NEXT:     # [sp, #4] = def.pseudo
; DIS-NEXT:   8:        e58d1004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %cmp = icmp slt i32 %a, %b

; ASM-NEXT:     mov     r0, #0
; DIS-NEXT:   c:        e3a00000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     ldr     r1, [sp, #8]
; DIS-NEXT:  10:        e59d1008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r2, [sp, #4]
; DIS-NEXT:  14:        e59d2004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x20
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     cmp     r1, r2
; DIS-NEXT:  18:        e1510002
; IASM-NEXT:    .byte 0x2
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x51
; IASM-NEXT:    .byte 0xe1

; ASM-NEXT:     movlt   r0, #1
; DIS-NEXT:  1c:        b3a00001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xb3

; ASM-NEXT:     strb    r0, [sp]
; ASM-NEXT:     # [sp] = def.pseudo
; DIS-NEXT:  20:        e5cd0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xcd
; IASM-NEXT:    .byte 0xe5

  br i1 %cmp, label %then, label %else

; ASM-NEXT:     ldrb    r0, [sp]
; DIS-NEXT:  24:        e5dd0000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xdd
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     tst     r0, #1
; DIS-NEXT:  28:        e3100001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     bne     .Lmult_fwd_branches$then
; DIS-NEXT:  2c:        1a000000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x1a

; ASM-NEXT:     b       .Lmult_fwd_branches$else
; DIS-NEXT:  30:        ea000000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xea

then:
; ASM-LABEL:.Lmult_fwd_branches$then:
; IASM-LABEL:.Lmult_fwd_branches$then:

  br label %end

; ASM-NEXT:     b       .Lmult_fwd_branches$end
; DIS-NEXT:  34:        ea000000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xea

else:
; ASM-LABEL:.Lmult_fwd_branches$else:
; IASM-LABEL:.Lmult_fwd_branches$else:

  br label %end
; ASM-NEXT:     b       .Lmult_fwd_branches$end
; DIS-NEXT:  38:        eaffffff
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xea


end:
; ASM-LABEL:.Lmult_fwd_branches$end:
; IASM-LABEL: .Lmult_fwd_branches$end:

  ret void

; ASM-NEXT:     add     sp, sp, #12
; DIS-NEXT:  3c:        e28dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe2

; ASM-NEXT:     bx      lr
; DIS-NEXT:  40:        e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
}
