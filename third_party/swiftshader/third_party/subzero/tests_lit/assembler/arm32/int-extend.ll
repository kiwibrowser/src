; Tests signed/unsigned extend to 32 bits.

; Show that we know how to translate add.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=ASM

; NOTE: We use -O2 to get rid of memory stores.

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 | FileCheck %s --check-prefix=DIS

define internal i32 @testUxtb(i32 %v) {
; ASM-LABEL:testUxtb:
; DIS-LABEL:00000000 <testUxtb>:
; IASM-LABEL:testUxtb:

entry:
; ASM-NEXT:.LtestUxtb$entry:
; IASM-NEXT:.LtestUxtb$entry:

  %v.b = trunc i32 %v to i8
  %res = zext i8 %v.b to i32

; ASM-NEXT:     uxtb    r0, r0
; DIS-NEXT:   0:        e6ef0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xef
; IASM-NEXT:    .byte 0xe6

  ret i32 %res
}

define internal i32 @testSxtb(i32 %v) {
; ASM-LABEL:testSxtb:
; DIS-LABEL:00000010 <testSxtb>:
; IASM-LABEL:testSxtb:

entry:
; ASM-NEXT:.LtestSxtb$entry:
; IASM-NEXT:.LtestSxtb$entry:

  %v.b = trunc i32 %v to i8
  %res = sext i8 %v.b to i32

; ASM-NEXT:     sxtb    r0, r0
; DIS-NEXT:  10:        e6af0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xaf
; IASM-NEXT:    .byte 0xe6

  ret i32 %res
}

define internal i32 @testUxth(i32 %v) {
; ASM-LABEL:testUxth:
; DIS-LABEL:00000020 <testUxth>:
; IASM-LABEL:testUxth:

entry:
; ASM-NEXT:.LtestUxth$entry:
; IASM-NEXT:.LtestUxth$entry:

  %v.h = trunc i32 %v to i16
  %res = zext i16 %v.h to i32

; ASM-NEXT:     uxth    r0, r0
; DIS-NEXT:  20:        e6ff0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xff
; IASM-NEXT:    .byte 0xe6

  ret i32 %res
}

define internal i32 @testSxth(i32 %v) {
; ASM-LABEL:testSxth:
; DIS-LABEL:00000030 <testSxth>:
; IASM-LABEL:testSxth:

entry:
; ASM-NEXT:.LtestSxth$entry:
; IASM-NEXT:.LtestSxth$entry:

  %v.h = trunc i32 %v to i16
  %res = sext i16 %v.h to i32

; ASM-NEXT:     sxth    r0, r0
; DIS-NEXT:  30:        e6bf0070
; IASM-NEXT:    .byte 0x70
; IASM-NEXT:    .byte 0x0
; IASM-NEXT:    .byte 0xbf
; IASM-NEXT:    .byte 0xe6

  ret i32 %res
}
