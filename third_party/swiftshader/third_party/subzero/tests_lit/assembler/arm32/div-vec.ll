; Show that we know how to translate vector division instructions.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal <4 x float> @testVdivFloat4(<4 x float> %v1, <4 x float> %v2) {
; ASM-LABEL: testVdivFloat4:
; DIS-LABEL: 00000000 <testVdivFloat4>:
; IASM-LABEL: testVdivFloat4:

entry:
  %res = fdiv <4 x float> %v1, %v2

; TODO(eholk): this code could be a lot better. Fix the code generator
; and update the test. Same for the rest of the tests.

; ASM:      vdiv.f32        s12, s12, s13
; ASM-NEXT: vmov.f32	    s8, s12
; ASM:      vdiv.f32        s12, s12, s13
; ASM-NEXT: vmov.f32	    s9, s12
; ASM:      vdiv.f32        s12, s12, s13
; ASM-NEXT: vmov.f32	    s10, s12
; ASM:      vdiv.f32        s0, s0, s4
; ASM-NEXT: vmov.f32	    s11, s0

; DIS:   8:	ee866a26
; DIS:  18:	ee866a26
; DIS:  28:	ee866a26
; DIS:  38:	ee800a02

; IASM-NOT:     vdiv

  ret <4 x float> %res
}

define internal <4 x i32> @testVdiv4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testVdiv4i32:
; DIS-LABEL: 00000050 <testVdiv4i32>:
; IASM-LABEL: testVdiv4i32:

entry:
  %res = udiv <4 x i32> %v1, %v2

; ASM:     udiv r0, r0, r1
; ASM:     udiv r0, r0, r1
; ASM:     udiv r0, r0, r1
; ASM:     udiv r0, r0, r1

; DIS:  64:	e730f110
; DIS:  80:	e730f110
; DIS:  9c:	e730f110
; DIS:  b8:	e730f110

; IASM-NOT:     udiv

  ret <4 x i32> %res
}

define internal <8 x i16> @testVdiv8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testVdiv8i16:
; DIS-LABEL: 000000d0 <testVdiv8i16>:
; IASM-LABEL: testVdiv8i16:

entry:
  %res = udiv <8 x i16> %v1, %v2

; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxth            r0, r0
; ASM:     uxth            r1, r1
; ASM:     udiv r0, r0, r1

; DIS:  e4:	e6ff0070
; DIS:  e8:	e6ff1071
; DIS:  ec:	e730f110
; DIS: 108:	e6ff0070
; DIS: 10c:	e6ff1071
; DIS: 110:	e730f110
; DIS: 12c:	e6ff0070
; DIS: 130:	e6ff1071
; DIS: 134:	e730f110
; DIS: 150:	e6ff0070
; DIS: 154:	e6ff1071
; DIS: 158:	e730f110
; DIS: 174:	e6ff0070
; DIS: 178:	e6ff1071
; DIS: 17c:	e730f110
; DIS: 198:	e6ff0070
; DIS: 19c:	e6ff1071
; DIS: 1a0:	e730f110
; DIS: 1bc:	e6ff0070
; DIS: 1c0:	e6ff1071
; DIS: 1c4:	e730f110
; DIS: 1e0:	e6ff0070
; DIS: 1e4:	e6ff1071
; DIS: 1e8:	e730f110

; IASM-NOT:     uxth
; IASM-NOT:     udiv

  ret <8 x i16> %res
}

define internal <16 x i8> @testVdiv16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testVdiv16i8:
; DIS-LABEL: 00000200 <testVdiv16i8>:
; IASM-LABEL: testVdiv16i8:

entry:
  %res = udiv <16 x i8> %v1, %v2

; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1
; ASM:     uxtb            r0, r0
; ASM:     uxtb            r1, r1
; ASM:     udiv r0, r0, r1

; DIS: 214:	e6ef0070
; DIS: 218:	e6ef1071
; DIS: 21c:	e730f110
; DIS: 238:	e6ef0070
; DIS: 23c:	e6ef1071
; DIS: 240:	e730f110
; DIS: 25c:	e6ef0070
; DIS: 260:	e6ef1071
; DIS: 264:	e730f110
; DIS: 280:	e6ef0070
; DIS: 284:	e6ef1071
; DIS: 288:	e730f110
; DIS: 2a4:	e6ef0070
; DIS: 2a8:	e6ef1071
; DIS: 2ac:	e730f110
; DIS: 2c8:	e6ef0070
; DIS: 2cc:	e6ef1071
; DIS: 2d0:	e730f110
; DIS: 2ec:	e6ef0070
; DIS: 2f0:	e6ef1071
; DIS: 2f4:	e730f110
; DIS: 310:	e6ef0070
; DIS: 314:	e6ef1071
; DIS: 318:	e730f110
; DIS: 334:	e6ef0070
; DIS: 338:	e6ef1071
; DIS: 33c:	e730f110
; DIS: 358:	e6ef0070
; DIS: 35c:	e6ef1071
; DIS: 360:	e730f110
; DIS: 37c:	e6ef0070
; DIS: 380:	e6ef1071
; DIS: 384:	e730f110
; DIS: 3a0:	e6ef0070
; DIS: 3a4:	e6ef1071
; DIS: 3a8:	e730f110
; DIS: 3c4:	e6ef0070
; DIS: 3c8:	e6ef1071
; DIS: 3cc:	e730f110
; DIS: 3e8:	e6ef0070
; DIS: 3ec:	e6ef1071
; DIS: 3f0:	e730f110
; DIS: 40c:	e6ef0070
; DIS: 410:	e6ef1071
; DIS: 414:	e730f110
; DIS: 430:	e6ef0070
; DIS: 434:	e6ef1071
; DIS: 438:	e730f110

; IASM-NOT:     uxtb
; IASM-NOT:     udiv

  ret <16 x i8> %res
}

define internal <4 x i32> @testSdiv4i32(<4 x i32> %v1, <4 x i32> %v2) {
; ASM-LABEL: testSdiv4i32:
; IASM-LABEL: testSdiv4i32:

entry:
  %res = sdiv <4 x i32> %v1, %v2

; ASM:     sdiv r0, r0, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sdiv r0, r0, r1

; IASM-NOT:     sdiv

  ret <4 x i32> %res
}

define internal <8 x i16> @testSdiv8i16(<8 x i16> %v1, <8 x i16> %v2) {
; ASM-LABEL: testSdiv8i16:
; IASM-LABEL: testSdiv8i16:

entry:
  %res = sdiv <8 x i16> %v1, %v2

; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxth            r0, r0
; ASM:     sxth            r1, r1
; ASM:     sdiv r0, r0, r1

; IASM-NOT:     sxth
; IASM-NOT:     sdiv

  ret <8 x i16> %res
}

define internal <16 x i8> @testSdiv16i8(<16 x i8> %v1, <16 x i8> %v2) {
; ASM-LABEL: testSdiv16i8:
; IASM-LABEL: testSdiv16i8:

entry:
  %res = sdiv <16 x i8> %v1, %v2

; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1
; ASM:     sxtb            r0, r0
; ASM:     sxtb            r1, r1
; ASM:     sdiv r0, r0, r1

; IASM-NOT:     sxtb
; IASM-NOT:     sdiv

  ret <16 x i8> %res
}
