; Show that we know how to translate LDR/LDRH (register) instructions.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %lc2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %lc2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %lc2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %lc2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Define some global arrays to access.
@ArrayInitPartial = internal global [40 x i8] c"<\00\00\00F\00\00\00P\00\00\00Z\00\00\00d\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00", align 4

@Arrays = internal constant <{ i32, [4 x i8] }> <{ i32 ptrtoint ([40 x i8]* @ArrayInitPartial to i32), [4 x i8] c"\14\00\00\00" }>, align 4

; Index elements of an array.
define internal i32 @IndexArray(i32 %WhichArray, i32 %Len) {
; ASM-LABEL:IndexArray:
; DIS-LABEL:00000000 <IndexArray>:
; IASM-LABEL:IndexArray:

entry:
; ASM-NEXT:.LIndexArray$entry:
; IASM-NEXT:.LIndexArray$entry:

  %gep_array = mul i32 %WhichArray, 8

; ASM-NEXT:     push    {r4}
; DIS-NEXT:   0:        e52d4004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0x2d
; IASM-NEXT:    .byte 0xe5

; ASM-NEXT:     lsl     r2, r0, #3
; DIS-NEXT:   4:        e1a02180
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0x21
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1

  %expanded1 = ptrtoint <{ i32, [4 x i8] }>* @Arrays to i32
  %gep = add i32 %expanded1, %gep_array

; ASM-NEXT:     movw    r3, #:lower16:Arrays
; DIS-NEXT:   8:        e3003000
; IASM-NEXT:    movw    r3, #:lower16:Arrays    @ .word e3003000

; ASM-NEXT:     movt    r3, #:upper16:Arrays
; DIS-NEXT:   c:        e3403000
; IASM-NEXT:    movt    r3, #:upper16:Arrays    @ .word e3403000

  %gep3 = add i32 %gep, 4

; ASM-NEXT:     add     r4, r3, #4
; DIS-NEXT:  10:        e2834004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0x83
; IASM-NEXT:    .byte 0xe2

; ***** Here is the use of a LDR (register) instruction.
  %gep3.asptr = inttoptr i32 %gep3 to i32*
  %v1 = load i32, i32* %gep3.asptr, align 1

; ASM-NEXT:     ldr     r4, [r4, r0, lsl #3]
; DIS-NEXT:  14:        e7944180
; IASM-NEXT:    .byte 0x80
; IASM-NEXT:    .byte 0x41
; IASM-NEXT:    .byte 0x94
; IASM-NEXT:    .byte 0xe7

  %Len.asptr3 = inttoptr i32 %Len to i32*
  store i32 %v1, i32* %Len.asptr3, align 1

; ASM-NEXT:     str     r4, [r1]
; DIS-NEXT:  18:        e5814000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x40
; IASM-NEXT:    .byte 0x81
; IASM-NEXT:    .byte 0xe5

  ; Now read the value as an i16 to test ldrh (register).
  %gep3.i16ptr = inttoptr i32 %gep3 to i16*
  %v16 = load i16, i16* %gep3.i16ptr, align 1

; ASM-NEXT:     add     r3, r3, #4
; DIS-NEXT:  1c:        e2833004
; IASM-NEXT:    .byte 0x4
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0x83
; IASM-NEXT:    .byte 0xe2

; ***** Here is the use of a LDRH (register) instruction.
; ASM-NEXT:     ldrh    r3, [r3, r2]
; DIS-NEXT:  20:        e19330b2
; IASM-NEXT:    .byte 0xb2
; IASM-NEXT:    .byte 0x30
; IASM-NEXT:    .byte 0x93
; IASM-NEXT:    .byte 0xe1

  %ret = sext i16 %v16 to i32
  ret i32 %ret
}
