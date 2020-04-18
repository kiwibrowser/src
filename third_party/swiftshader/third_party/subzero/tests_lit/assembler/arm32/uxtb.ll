; Test the UXTB and UXTH instructions.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

define internal i32 @_Z7testAddhh(i32 %a, i32 %b) {
; ASM-LABEL: _Z7testAddhh:
; DIS-LABEL: 00000000 <_Z7testAddhh>:
; IASM-LABEL: _Z7testAddhh:

entry:

; ASM-NEXT: .L_Z7testAddhh$entry:
; IASM-NEXT: .L_Z7testAddhh$entry:

  %a.arg_trunc = trunc i32 %a to i8
  %conv = zext i8 %a.arg_trunc to i32

; ASM-NEXT:     uxtb    r0, r0
; DIS-NEXT:    0:       e6ef0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xef
; IASM-NEXT:    .byte 0xe6

  %b.arg_trunc = trunc i32 %b to i8
  %conv1 = zext i8 %b.arg_trunc to i32

; ASM-NEXT:     uxtb    r1, r1
; DIS-NEXT:    4:       e6ef1071
; IASM-NEXT:    .byte 0x71
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xef
; IASM-NEXT:    .byte 0xe6

  %add = add i32 %conv1, %conv

; ASM-NEXT:     add     r1, r1, r0
; DIS-NEXT:    8:       e0811000
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0x81
; IASM-NEXT:    .byte 0xe0

  %conv2 = trunc i32 %add to i16
  %conv2.ret_ext = zext i16 %conv2 to i32

; ASM-NEXT:     uxth    r1, r1
; DIS-NEXT:   c:        e6ff1071
; IASM-NEXT:    .byte 0x71
; IASM-NEXT:    .byte 0x10
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xe6

  ret i32 %conv2.ret_ext

; ASM-NEXT:     mov     r0, r1
; DIS-NEXT:   10:       e1a00001
; IASM-NEXT:    .byte 0x1
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xa0
; IASM-NEXT:    .byte 0xe1


; ASM-NEXT:     bx      lr
; DIS-NEXT:   14:       e12fff1e
; IASM-NEXT:    .byte 0x1e
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0x2f
; IASM-NEXT:    .byte 0xe1

}
