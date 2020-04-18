; Tests assembly of ldrex and strex instructions

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

declare i8 @llvm.nacl.atomic.rmw.i8(i32, i8*, i8, i32)

declare i16 @llvm.nacl.atomic.rmw.i16(i32, i16*, i16, i32)

declare i32 @llvm.nacl.atomic.rmw.i32(i32, i32*, i32, i32) #0

declare i64 @llvm.nacl.atomic.rmw.i64(i32, i64*, i64, i32) #0

define internal i32 @testI8Form(i32 %ptr, i32 %a) {
; ASM-LABEL:testI8Form:
; DIS-LABEL:<testI8Form>:
; IASM-LABEL:testI8Form:

entry:
  %ptr.asptr = inttoptr i32 %ptr to i8*
  %a.arg_trunc = trunc i32 %a to i8

  %v = call i8 @llvm.nacl.atomic.rmw.i8(i32 1, i8* %ptr.asptr,
                                        i8 %a.arg_trunc, i32 6)

; ****** Example of dmb *******
; ASM:          dmb     sy
; DIS:     1c:  f57ff05f
; IASM:         .byte 0x5f
; IASM-NEXT:    .byte 0xf0
; IASM-NEXT:    .byte 0x7f
; IASM-NEXT:    .byte 0xf5

; ***** Example of ldrexb *****
; ASM:          ldrexb  r1, [r2]
; DIS:     24:  e1d21f9f
; IASM:         .byte 0x9f
; IASM-NEXT:    .byte 0x1f
; IASM-NEXT:    .byte 0xd2
; IASM-NEXT:    .byte 0xe1

; ***** Example of strexb *****
; ASM:          strexb  r4, r3, [r2]
; DIS:     2c:  e1c24f93
; IASM:         .byte 0x93
; IASM-NEXT:    .byte 0x4f
; IASM-NEXT:    .byte 0xc2
; IASM-NEXT:    .byte 0xe1

  %retval = zext i8 %v to i32
  ret i32 %retval
}

define internal i32 @testI16Form(i32 %ptr, i32 %a) {
; ASM-LABEL:testI16Form:
; DIS-LABEL:<testI16Form>:
; IASM-LABEL:testI16Form:

entry:
  %ptr.asptr = inttoptr i32 %ptr to i16*
  %a.arg_trunc = trunc i32 %a to i16

  %v = call i16 @llvm.nacl.atomic.rmw.i16(i32 1, i16* %ptr.asptr,
                                          i16 %a.arg_trunc, i32 6)
; ***** Example of ldrexh *****
; ASM:          ldrexh  r1, [r2]
; DIS:     84:  e1f21f9f
; IASM:         .byte 0x9f
; IASM-NEXT:    .byte 0x1f
; IASM-NEXT:    .byte 0xf2
; IASM-NEXT:    .byte 0xe1

; ***** Example of strexh *****
; ASM:          strexh  r4, r3, [r2]
; DIS:     8c:  e1e24f93
; IASM:         .byte 0x93
; IASM-NEXT:    .byte 0x4f
; IASM-NEXT:    .byte 0xe2
; IASM-NEXT:    .byte 0xe1

  %retval = zext i16 %v to i32
  ret i32 %retval
}

define internal i32 @testI32Form(i32 %ptr, i32 %a) {
; ASM-LABEL:testI32Form:
; DIS-LABEL:<testI32Form>:
; IASM-LABEL:testI32Form:

entry:
  %ptr.asptr = inttoptr i32 %ptr to i32*
  %v = call i32 @llvm.nacl.atomic.rmw.i32(i32 1, i32* %ptr.asptr,
                                          i32 %a, i32 6)

; ***** Example of ldrex *****
; ASM:          ldrex   r1, [r2]
; DIS:     dc:  e1921f9f
; IASM:         .byte 0x9f
; IASM-NEXT:    .byte 0x1f
; IASM-NEXT:    .byte 0x92
; IASM-NEXT:    .byte 0xe1

; ***** Example of strex *****
; ASM:          strex   r4, r3, [r2]
; DIS:     e4:  e1824f93
; IASM:         .byte 0x93
; IASM-NEXT:    .byte 0x4f
; IASM-NEXT:    .byte 0x82
; IASM-NEXT:    .byte 0xe1

  ret i32 %v
}

define internal i64 @testI64Form(i32 %ptr, i64 %a) {
; ASM-LABEL:testI64Form:
; DIS-LABEL:<testI64Form>:
; IASM-LABEL:testI64Form:

entry:
  %ptr.asptr = inttoptr i32 %ptr to i64*
  %v = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr.asptr,
                                          i64 %a, i32 6)

; ***** Example of ldrexd *****
; ASM:          ldrexd  r4, r5, [r6]
; DIS:     13c: e1b64f9f
; IASM:         .byte 0x9f
; IASM-NEXT:    .byte 0x4f
; IASM-NEXT:    .byte 0xb6
; IASM-NEXT:    .byte 0xe1

; ***** Example of strexd *****
; ASM:          strexd  r4, r0, r1, [r6]
; DIS:     158: e1a64f90
; IASM:         .byte 0x90
; IASM-NEXT:    .byte 0x4f
; IASM-NEXT:    .byte 0xa6
; IASM-NEXT:    .byte 0xe1

  ret i64 %v
}
