; Show that we handle constants in movw and mvt, when it isn't represented as
; ConstantRelocatable (see mov-imm.ll for the ConstantRelocatable case).

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   --test-stack-extra 4084 | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --test-stack-extra 4084 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   --test-stack-extra 4084 | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 --test-stack-extra 4084 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i32 @foo(i32 %x) {
entry:

; ASM-LABEL: foo:
; DIS-LABEL: 00000000 <foo>:
; IASM-LABEL: foo:

; ASM-NEXT: .Lfoo$entry:
; IASM-NEXT: .Lfoo$entry:

; ASM-NEXT:     movw    ip, #4092
; DIS-NEXT:    0:       e300cffc
; IASM-NEXT:    .byte 0xfc
; IASM-NEXT:    .byte 0xcf
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     sub     sp, sp, ip
; DIS-NEXT:    4:       e04dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe0

; ASM-NEXT:     str     r0, [sp, #4088]
; DIS-NEXT:    8:       e58d0ff8
; IASM-NEXT:    .byte 0xf8
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     # [sp, #4088] = def.pseudo

  %mul = mul i32 %x, %x

; ASM-NEXT:     ldr     r0, [sp, #4088]
; DIS-NEXT:    c:       e59d0ff8
; IASM-NEXT:    .byte 0xf8
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     ldr     r1, [sp, #4088]
; DIS-NEXT:   10:       e59d1ff8
; IASM-NEXT:    .byte 0xf8
; IASM-NEXT:    .byte 0x1f
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     mul     r0, r0, r1
; DIS-NEXT:   14:       e0000190
; IASM-NEXT:    .byte 0x90
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe0

; ASM-NEXT:     str     r0, [sp, #4084]
; DIS-NEXT:   18:       e58d0ff4
; IASM-NEXT:    .byte 0xf4
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     # [sp, #4084] = def.pseudo

  ret i32 %mul

; ASM-NEXT:     ldr     r0, [sp, #4084]
; DIS-NEXT:   1c:       e59d0ff4
; IASM-NEXT:    .byte 0xf4
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     movw    ip, #4092
; DIS-NEXT:   20:       e300cffc
; IASM-NEXT:    .byte 0xfc
; IASM-NEXT:    .byte 0xcf
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     add     sp, sp, ip
; DIS-NEXT:   24:       e08dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe0

; ASM-NEXT:     bx      lr
; DIS-NEXT:   28:       e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

}

define internal void @saveConstI32(i32 %loc) {
; ASM-LABEL:saveConstI32:
; DIS-LABEL:00000030 <saveConstI32>:
; IASM-LABEL:saveConstI32:

entry:
; ASM-NEXT:.LsaveConstI32$entry:
; IASM-NEXT:.LsaveConstI32$entry:

; ASM-NEXT:     movw    ip, #4088
; DIS-NEXT:  30:        e300cff8
; IASM-NEXT:    .byte 0xf8
; IASM-NEXT:    .byte 0xcf
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     sub     sp, sp, ip
; DIS-NEXT:  34:        e04dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x4d
; IASM-NEXT:    .byte 0xe0

; ASM-NEXT:     str     r0, [sp, #4084]
; ASM-NEXT:     # [sp, #4084] = def.pseudo
; DIS-NEXT:  38:        e58d0ff4
; IASM-NEXT:    .byte 0xf4
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe5

  %loc.asptr = inttoptr i32 %loc to i32*
  store i32 524289, i32* %loc.asptr, align 1

; ASM-NEXT:     ldr     r0, [sp, #4084]
; DIS-NEXT:  3c:        e59d0ff4
; IASM-NEXT:    .byte 0xf4
; IASM-NEXT:    .byte 0xf
; IASM-NEXT:    .byte 0x9d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     movw     r1, #1
; DIS-NEXT:  40:        e3001001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     movt    r1, #8
; DIS-NEXT:  44:        e3401008
; IASM-NEXT:    .byte 0x8
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     str     r1, [r0]
; DIS-NEXT:  48:        e5801000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0xe5

  ret void

; ASM-NEXT:     movw    ip, #4088
; DIS-NEXT:  4c:        e300cff8
; IASM-NEXT:    .byte 0xf8
; IASM-NEXT:    .byte 0xcf
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xe3

; ASM-NEXT:     add     sp, sp, ip
; DIS-NEXT:  50:        e08dd00c
; IASM-NEXT:    .byte 0xc
; IASM-NEXT:    .byte 0xd0
; IASM-NEXT:    .byte 0x8d
; IASM-NEXT:    .byte 0xe0

; ASM-NEXT:     bx      lr
; DIS-NEXT:  54:        e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1
}
