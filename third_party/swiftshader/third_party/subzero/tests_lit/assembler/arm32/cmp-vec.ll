; Test that we handle icmp and fcmp on vectors.

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

define internal <4 x i32> @cmpEqV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpEqV4I32:
; DIS-LABEL:00000000 <cmpEqV4I32>:
; IASM-LABEL:cmpEqV4I32:

entry:
  %cmp = icmp eq <4 x i32> %a, %b

; ASM:         vceq.i32 q0, q0, q1
; DIS:      0: f3200852
; IASM-NOT:    vceq

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpNeV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpNeV4I32:
; DIS-LABEL:00000010 <cmpNeV4I32>:
; IASM-LABEL:cmpNeV4I32:

entry:
  %cmp = icmp ne <4 x i32> %a, %b

; ASM:          vceq.i32 q0, q0, q1
; ASM-NEXT:     vmvn.i32 q0, q0
; DIS:      10: f3200852
; DIS-NEXT: 14: f3b005c0
; IASM-NOT:     vceq
; IASM-NOT:     vmvn

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUgtV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUgtV4I32:
; DIS-LABEL:00000030 <cmpUgtV4I32>:
; IASM-LABEL:cmpUgtV4I32:

entry:
  %cmp = icmp ugt <4 x i32> %a, %b

; ASM:          vcgt.u32 q0, q0, q1
; DIS:      30: f3200342
; IASM-NOT:     vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUgeV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUgeV4I32:
; DIS-LABEL:00000040 <cmpUgeV4I32>:
; IASM-LABEL:cmpUgeV4I32:

entry:
  %cmp = icmp uge <4 x i32> %a, %b

; ASM:          vcge.u32 q0, q0, q1
; DIS:      40: f3200352
; IASM-NOT:     vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUltV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUltV4I32:
; DIS-LABEL:00000050 <cmpUltV4I32>:
; IASM-LABEL:cmpUltV4I32:

entry:
  %cmp = icmp ult <4 x i32> %a, %b

; ASM:          vcgt.u32 q1, q1, q0
; DIS:      50: f3222340
; IASM-NOT:     vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUleV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUleV4I32:
; DIS-LABEL:00000070 <cmpUleV4I32>:
; IASM-LABEL:cmpUleV4I32:

entry:
  %cmp = icmp ule <4 x i32> %a, %b

; ASM:          vcge.u32 q1, q1, q0
; DIS:      70: f3222350
; IASM-NOT:     vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSgtV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSgtV4I32:
; DIS-LABEL:00000090 <cmpSgtV4I32>:
; IASM-LABEL:cmpSgtV4I32:

entry:
  %cmp = icmp sgt <4 x i32> %a, %b

; ASM:          vcgt.s32 q0, q0, q1
; DIS:      90: f2200342
; IASM-NOT:     vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSgeV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSgeV4I32:
; DIS-LABEL:000000a0 <cmpSgeV4I32>:
; IASM-LABEL:cmpSgeV4I32:

entry:
  %cmp = icmp sge <4 x i32> %a, %b

; ASM:          vcge.s32 q0, q0, q1
; DIS:      a0: f2200352
; IASM-NOT:     vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSltV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSltV4I32:
; DIS-LABEL:000000b0 <cmpSltV4I32>:
; IASM-LABEL:cmpSltV4I32:

entry:
  %cmp = icmp slt <4 x i32> %a, %b

; ASM:          vcgt.s32 q1, q1, q0
; DIS:      b0: f2222340
; IASM-NOT:     vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSleV4I32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSleV4I32:
; DIS-LABEL:000000d0 <cmpSleV4I32>:
; IASM-LABEL:cmpSleV4I32:

entry:
  %cmp = icmp sle <4 x i32> %a, %b

; ASM:          vcge.s32 q1, q1, q0
; DIS:      d0: f2222350
; IASM-NOT:     vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpEqV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpEqV4I1:
; DIS-LABEL:000000f0 <cmpEqV4I1>:
; IASM-LABEL:cmpEqV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp eq <4 x i1> %a1, %b1

; ASM:          vshl.u32 q0, q0, #31
; ASM-NEXT:     vshl.u32 q1, q1, #31
; ASM-NEXT:     vceq.i32 q0, q0, q1
; DIS:      f0: f2bf0550
; DIS-NEXT: f4: f2bf2552
; DIS-NEXT: f8: f3200852
; IASM-NOT:     vshl
; IASM-NOT:     vceq

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpNeV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpNeV4I1:
; DIS-LABEL:00000110 <cmpNeV4I1>:
; IASM-LABEL:cmpNeV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp ne <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vceq.i32 q0, q0, q1
; ASM-NEXT:      vmvn.i32 q0, q0
; DIS:      110: f2bf0550
; DIS-NEXT: 114: f2bf2552
; DIS-NEXT: 118: f3200852
; DIS-NEXT: 11c: f3b005c0
; IASM-NOT:      vshl
; IASM-NOT:      vceq
; IASM-NOT:      vmvn

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUgtV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUgtV4I1:
; DIS-LABEL:00000130 <cmpUgtV4I1>:
; IASM-LABEL:cmpUgtV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp ugt <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcgt.u32 q0, q0, q1
; DIS:      130: f2bf0550
; DIS-NEXT: 134: f2bf2552
; DIS-NEXT: 138: f3200342
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUgeV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUgeV4I1:
; DIS-LABEL:00000150 <cmpUgeV4I1>:
; IASM-LABEL:cmpUgeV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp uge <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcge.u32 q0, q0, q1
; DIS:      150: f2bf0550
; DIS-NEXT: 154: f2bf2552
; DIS-NEXT: 158: f3200352
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUltV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUltV4I1:
; DIS-LABEL:00000170 <cmpUltV4I1>:
; IASM-LABEL:cmpUltV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp ult <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcgt.u32 q1, q1, q0
; DIS:      170: f2bf0550
; DIS-NEXT: 174: f2bf2552
; DIS-NEXT: 178: f3222340
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpUleV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpUleV4I1:
; DIS-LABEL:00000190 <cmpUleV4I1>:
; IASM-LABEL:cmpUleV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp ule <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcge.u32 q1, q1, q0
; DIS:      190: f2bf0550
; DIS-NEXT: 194: f2bf2552
; DIS-NEXT: 198: f3222350
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSgtV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSgtV4I1:
; DIS-LABEL:000001b0 <cmpSgtV4I1>:
; IASM-LABEL:cmpSgtV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp sgt <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcgt.s32 q0, q0, q1
; DIS:      1b0: f2bf0550
; DIS-NEXT: 1b4: f2bf2552
; DIS-NEXT: 1b8: f2200342
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSgeV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSgeV4I1:
; DIS-LABEL:000001d0 <cmpSgeV4I1>:
; IASM-LABEL:cmpSgeV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp sge <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcge.s32 q0, q0, q1
; DIS:      1d0: f2bf0550
; DIS-NEXT: 1d4: f2bf2552
; DIS-NEXT: 1d8: f2200352
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSltV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSltV4I1:
; DIS-LABEL:000001f0 <cmpSltV4I1>:
; IASM-LABEL:cmpSltV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp slt <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcgt.s32 q1, q1, q0
; DIS:      1f0: f2bf0550
; DIS-NEXT: 1f4: f2bf2552
; DIS-NEXT: 1f8: f2222340
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <4 x i32> @cmpSleV4I1(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:cmpSleV4I1:
; DIS-LABEL:00000210 <cmpSleV4I1>:
; IASM-LABEL:cmpSleV4I1:

entry:
  %a1 = trunc <4 x i32> %a to <4 x i1>
  %b1 = trunc <4 x i32> %b to <4 x i1>
  %cmp = icmp sle <4 x i1> %a1, %b1

; ASM:           vshl.u32 q0, q0, #31
; ASM-NEXT:      vshl.u32 q1, q1, #31
; ASM-NEXT:      vcge.s32 q1, q1, q0
; DIS:      210: f2bf0550
; DIS-NEXT: 214: f2bf2552
; DIS-NEXT: 218: f2222350
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %cmp.ret_ext
}

define internal <8 x i16> @cmpEqV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpEqV8I16:
; DIS-LABEL:00000230 <cmpEqV8I16>:
; IASM-LABEL:cmpEqV8I16:

entry:
  %cmp = icmp eq <8 x i16> %a, %b

; ASM:           vceq.i16 q0, q0, q1
; DIS:      230: f3100852
; IASM-NOT:      vceq

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpNeV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpNeV8I16:
; DIS-LABEL:00000240 <cmpNeV8I16>:
; IASM-LABEL:cmpNeV8I16:

entry:
  %cmp = icmp ne <8 x i16> %a, %b

; ASM:           vceq.i16 q0, q0, q1
; ASM-NEXT:      vmvn.i16 q0, q0
; DIS:      240: f3100852
; DIS-NEXT: 244: f3b005c0
; IASM-NOT:      vceq
; IASM-NOT:      vmvn

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUgtV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUgtV8I16:
; DIS-LABEL:00000260 <cmpUgtV8I16>:
; IASM-LABEL:cmpUgtV8I16:

entry:
  %cmp = icmp ugt <8 x i16> %a, %b

; ASM:           vcgt.u16 q0, q0, q1
; DIS:      260: f3100342
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUgeV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUgeV8I16:
; DIS-LABEL:00000270 <cmpUgeV8I16>:
; IASM-LABEL:cmpUgeV8I16:

entry:
  %cmp = icmp uge <8 x i16> %a, %b

; ASM:           vcge.u16 q0, q0, q1
; DIS:      270: f3100352
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUltV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUltV8I16:
; DIS-LABEL:00000280 <cmpUltV8I16>:
; IASM-LABEL:cmpUltV8I16:

entry:
  %cmp = icmp ult <8 x i16> %a, %b

; ASM:           vcgt.u16 q1, q1, q0
; DIS:      280: f3122340
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUleV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUleV8I16:
; DIS-LABEL:000002a0 <cmpUleV8I16>:
; IASM-LABEL:cmpUleV8I16:

entry:
  %cmp = icmp ule <8 x i16> %a, %b

; ASM:           vcge.u16 q1, q1, q0
; DIS:      2a0: f3122350
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSgtV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSgtV8I16:
; DIS-LABEL:000002c0 <cmpSgtV8I16>:
; IASM-LABEL:cmpSgtV8I16:

entry:
  %cmp = icmp sgt <8 x i16> %a, %b

; ASM:           vcgt.s16 q0, q0, q1
; DIS:      2c0: f2100342
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSgeV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSgeV8I16:
; DIS-LABEL:000002d0 <cmpSgeV8I16>:
; IASM-LABEL:cmpSgeV8I16:

entry:
  %cmp = icmp sge <8 x i16> %a, %b

; ASM:           vcge.s16 q0, q0, q1
; DIS:      2d0: f2100352
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSltV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSltV8I16:
; DIS-LABEL:000002e0 <cmpSltV8I16>:
; IASM-LABEL:cmpSltV8I16:

entry:
  %cmp = icmp slt <8 x i16> %a, %b

; ASM:           vcgt.s16 q1, q1, q0
; DIS:      2e0: f2122340
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSleV8I16(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSleV8I16:
; DIS-LABEL:00000300 <cmpSleV8I16>:
; IASM-LABEL:cmpSleV8I16:

entry:
  %cmp = icmp sle <8 x i16> %a, %b

; ASM:           vcge.s16 q1, q1, q0
; DIS:      300: f2122350
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpEqV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpEqV8I1:
; DIS-LABEL:00000320 <cmpEqV8I1>:
; IASM-LABEL:cmpEqV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp eq <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vceq.i16 q0, q0, q1
; DIS:      320: f29f0550
; DIS-NEXT: 324: f29f2552
; DIS-NEXT: 328: f3100852
; IASM-NOT:      vshl
; IASM-NOT:      vceq

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpNeV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpNeV8I1:
; DIS-LABEL:00000340 <cmpNeV8I1>:
; IASM-LABEL:cmpNeV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp ne <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vceq.i16 q0, q0, q1
; ASM-NEXT:      vmvn.i16 q0, q0
; DIS:      340: f29f0550
; DIS-NEXT: 344: f29f2552
; DIS-NEXT: 348: f3100852
; DIS-NEXT: 34c: f3b005c0
; IASM-NOT:      vshl
; IASM-NOT:      vceq
; IASM-NOT:      vmvn

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUgtV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUgtV8I1:
; DIS-LABEL:00000360 <cmpUgtV8I1>:
; IASM-LABEL:cmpUgtV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp ugt <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcgt.u16 q0, q0, q1
; DIS:      360: f29f0550
; DIS-NEXT: 364: f29f2552
; DIS-NEXT: 368: f3100342
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUgeV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUgeV8I1:
; DIS-LABEL:00000380 <cmpUgeV8I1>:
; IASM-LABEL:cmpUgeV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp uge <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcge.u16 q0, q0, q1
; DIS:      380: f29f0550
; DIS-NEXT: 384: f29f2552
; DIS-NEXT: 388: f3100352
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUltV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUltV8I1:
; DIS-LABEL:000003a0 <cmpUltV8I1>:
; IASM-LABEL:cmpUltV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp ult <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcgt.u16 q1, q1, q0
; DIS:      3a0: f29f0550
; DIS-NEXT: 3a4: f29f2552
; DIS-NEXT: 3a8: f3122340
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpUleV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpUleV8I1:
; DIS-LABEL:000003c0 <cmpUleV8I1>:
; IASM-LABEL:cmpUleV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp ule <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcge.u16 q1, q1, q0
; DIS:      3c0: f29f0550
; DIS-NEXT: 3c4: f29f2552
; DIS-NEXT: 3c8: f3122350
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSgtV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSgtV8I1:
; DIS-LABEL:000003e0 <cmpSgtV8I1>:
; IASM-LABEL:cmpSgtV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp sgt <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcgt.s16 q0, q0, q1
; DIS:      3e0: f29f0550
; DIS-NEXT: 3e4: f29f2552
; DIS-NEXT: 3e8: f2100342
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSgeV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSgeV8I1:
; DIS-LABEL:00000400 <cmpSgeV8I1>:
; IASM-LABEL:cmpSgeV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp sge <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcge.s16 q0, q0, q1
; DIS:      400: f29f0550
; DIS-NEXT: 404: f29f2552
; DIS-NEXT: 408: f2100352
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSltV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSltV8I1:
; DIS-LABEL:00000420 <cmpSltV8I1>:
; IASM-LABEL:cmpSltV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp slt <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcgt.s16 q1, q1, q0
; DIS:      420: f29f0550
; DIS-NEXT: 424: f29f2552
; DIS-NEXT: 428: f2122340
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <8 x i16> @cmpSleV8I1(<8 x i16> %a, <8 x i16> %b) {
; ASM-LABEL:cmpSleV8I1:
; DIS-LABEL:00000440 <cmpSleV8I1>:
; IASM-LABEL:cmpSleV8I1:

entry:
  %a1 = trunc <8 x i16> %a to <8 x i1>
  %b1 = trunc <8 x i16> %b to <8 x i1>
  %cmp = icmp sle <8 x i1> %a1, %b1

; ASM:           vshl.u16 q0, q0, #15
; ASM-NEXT:      vshl.u16 q1, q1, #15
; ASM-NEXT:      vcge.s16 q1, q1, q0
; DIS:      440: f29f0550
; DIS-NEXT: 444: f29f2552
; DIS-NEXT: 448: f2122350
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <8 x i1> %cmp to <8 x i16>
  ret <8 x i16> %cmp.ret_ext
}

define internal <16 x i8> @cmpEqV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpEqV16I8:
; DIS-LABEL:00000460 <cmpEqV16I8>:
; IASM-LABEL:cmpEqV16I8:

entry:
  %cmp = icmp eq <16 x i8> %a, %b

; ASM:           vceq.i8 q0, q0, q1
; DIS:      460: f3000852
; IASM-NOT:      vceq

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpNeV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpNeV16I8:
; DIS-LABEL:00000470 <cmpNeV16I8>:
; IASM-LABEL:cmpNeV16I8:

entry:
  %cmp = icmp ne <16 x i8> %a, %b

; ASM:           vceq.i8 q0, q0, q1
; ASM-NEXT:      vmvn.i8 q0, q0
; DIS:      470: f3000852
; DIS-NEXT: 474: f3b005c0
; IASM-NOT:      vceq
; IASM-NOT:      vmvn

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUgtV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUgtV16I8:
; DIS-LABEL:00000490 <cmpUgtV16I8>:
; IASM-LABEL:cmpUgtV16I8:

entry:
  %cmp = icmp ugt <16 x i8> %a, %b

; ASM:           vcgt.u8 q0, q0, q1
; DIS:      490: f3000342
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUgeV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUgeV16I8:
; DIS-LABEL:000004a0 <cmpUgeV16I8>:
; IASM-LABEL:cmpUgeV16I8:

entry:
  %cmp = icmp uge <16 x i8> %a, %b

; ASM:           vcge.u8 q0, q0, q1
; DIS:      4a0: f3000352
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUltV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUltV16I8:
; DIS-LABEL:000004b0 <cmpUltV16I8>:
; IASM-LABEL:cmpUltV16I8:

entry:
  %cmp = icmp ult <16 x i8> %a, %b

; ASM:           vcgt.u8 q1, q1, q0
; DIS:      4b0: f3022340
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUleV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUleV16I8:
; DIS-LABEL:000004d0 <cmpUleV16I8>:
; IASM-LABEL:cmpUleV16I8:

entry:
  %cmp = icmp ule <16 x i8> %a, %b

; ASM:           vcge.u8 q1, q1, q0
; DIS:      4d0: f3022350
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSgtV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSgtV16I8:
; DIS-LABEL:000004f0 <cmpSgtV16I8>:
; IASM-LABEL:cmpSgtV16I8:

entry:
  %cmp = icmp sgt <16 x i8> %a, %b

; ASM:           vcgt.s8 q0, q0, q1
; DIS:      4f0: f2000342
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSgeV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSgeV16I8:
; DIS-LABEL:00000500 <cmpSgeV16I8>:
; IASM-LABEL:cmpSgeV16I8:

entry:
  %cmp = icmp sge <16 x i8> %a, %b

; ASM:           vcge.s8 q0, q0, q1
; DIS:      500: f2000352
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSltV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSltV16I8:
; DIS-LABEL:00000510 <cmpSltV16I8>:
; IASM-LABEL:cmpSltV16I8:

entry:
  %cmp = icmp slt <16 x i8> %a, %b

; ASM:           vcgt.s8 q1, q1, q0
; DIS:      510: f2022340
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSleV16I8(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSleV16I8:
; DIS-LABEL:00000530 <cmpSleV16I8>:
; IASM-LABEL:cmpSleV16I8:

entry:
  %cmp = icmp sle <16 x i8> %a, %b

; ASM:           vcge.s8 q1, q1, q0
; DIS:      530: f2022350
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpEqV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpEqV16I1:
; DIS-LABEL:00000550 <cmpEqV16I1>:
; IASM-LABEL:cmpEqV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp eq <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vceq.i8 q0, q0, q1
; DIS:      550: f28f0550
; DIS-NEXT: 554: f28f2552
; DIS-NEXT: 558: f3000852
; IASM-NOT:      vshl
; IASM-NOT:      vceq

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpNeV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpNeV16I1:
; DIS-LABEL:00000570 <cmpNeV16I1>:
; IASM-LABEL:cmpNeV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp ne <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vceq.i8 q0, q0, q1
; ASM-NEXT:      vmvn.i8 q0, q0
; DIS:      570: f28f0550
; DIS-NEXT: 574: f28f2552
; DIS-NEXT: 578: f3000852
; DIS-NEXT: 57c: f3b005c0
; IASM-NOT:      vshl
; IASM-NOT:      vceq
; IASM-NOT:      vmvn

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUgtV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUgtV16I1:
; DIS-LABEL:00000590 <cmpUgtV16I1>:
; IASM-LABEL:cmpUgtV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp ugt <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcgt.u8 q0, q0, q1
; DIS:      590: f28f0550
; DIS-NEXT: 594: f28f2552
; DIS-NEXT: 598: f3000342
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUgeV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUgeV16I1:
; DIS-LABEL:000005b0 <cmpUgeV16I1>:
; IASM-LABEL:cmpUgeV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp uge <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcge.u8 q0, q0, q1
; DIS:      5b0: f28f0550
; DIS-NEXT: 5b4: f28f2552
; DIS-NEXT: 5b8: f3000352
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUltV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUltV16I1:
; DIS-LABEL:000005d0 <cmpUltV16I1>:
; IASM-LABEL:cmpUltV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp ult <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcgt.u8 q1, q1, q0
; DIS:      5d0: f28f0550
; DIS-NEXT: 5d4: f28f2552
; DIS-NEXT: 5d8: f3022340
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpUleV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpUleV16I1:
; DIS-LABEL:000005f0 <cmpUleV16I1>:
; IASM-LABEL:cmpUleV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp ule <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcge.u8 q1, q1, q0
; DIS:      5f0: f28f0550
; DIS-NEXT: 5f4: f28f2552
; DIS-NEXT: 5f8: f3022350
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSgtV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSgtV16I1:
; DIS-LABEL:00000610 <cmpSgtV16I1>:
; IASM-LABEL:cmpSgtV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp sgt <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcgt.s8 q0, q0, q1
; DIS:      610: f28f0550
; DIS-NEXT: 614: f28f2552
; DIS-NEXT: 618: f2000342
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSgeV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSgeV16I1:
; DIS-LABEL:00000630 <cmpSgeV16I1>:
; IASM-LABEL:cmpSgeV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp sge <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcge.s8 q0, q0, q1
; DIS:      630: f28f0550
; DIS-NEXT: 634: f28f2552
; DIS-NEXT: 638: f2000352
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSltV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSltV16I1:
; DIS-LABEL:00000650 <cmpSltV16I1>:
; IASM-LABEL:cmpSltV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp slt <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcgt.s8 q1, q1, q0
; DIS:      650: f28f0550
; DIS-NEXT: 654: f28f2552
; DIS-NEXT: 658: f2022340
; IASM-NOT:      vshl
; IASM-NOT:      vcgt

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <16 x i8> @cmpSleV16I1(<16 x i8> %a, <16 x i8> %b) {
; ASM-LABEL:cmpSleV16I1:
; DIS-LABEL:00000670 <cmpSleV16I1>:
; IASM-LABEL:cmpSleV16I1:

entry:
  %a1 = trunc <16 x i8> %a to <16 x i1>
  %b1 = trunc <16 x i8> %b to <16 x i1>
  %cmp = icmp sle <16 x i1> %a1, %b1

; ASM:           vshl.u8 q0, q0, #7
; ASM-NEXT:      vshl.u8 q1, q1, #7
; ASM-NEXT:      vcge.s8 q1, q1, q0
; DIS:      670: f28f0550
; DIS-NEXT: 674: f28f2552
; DIS-NEXT: 678: f2022350
; IASM-NOT:      vshl
; IASM-NOT:      vcge

  %cmp.ret_ext = zext <16 x i1> %cmp to <16 x i8>
  ret <16 x i8> %cmp.ret_ext
}

define internal <4 x i32> @cmpFalseV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpFalseV4Float:
; DIS-LABEL:00000690 <cmpFalseV4Float>:
; IASM-LABEL:cmpFalseV4Float:

entry:
  %cmp = fcmp false <4 x float> %a, %b

; ASM:           vmov.i32 q0, #0
; DIS:      690: f2800050
; IASM-NOT:      vmov

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpOeqV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpOeqV4Float:
; DIS-LABEL:000006a0 <cmpOeqV4Float>:
; IASM-LABEL:cmpOeqV4Float:

entry:
  %cmp = fcmp oeq <4 x float> %a, %b

; ASM:           vceq.f32 q0, q0, q1
; DIS:      6a0: f2000e42
; IASM-NOT:      vceq

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpOgtV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpOgtV4Float:
; DIS-LABEL:000006b0 <cmpOgtV4Float>:
; IASM-LABEL:cmpOgtV4Float:

entry:
  %cmp = fcmp ogt <4 x float> %a, %b

; ASM:           vcgt.f32 q0, q0, q1
; DIS:      6b0: f3200e42
; IASM-NOT:      vcgt

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpOgeV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpOgeV4Float:
; DIS-LABEL:000006c0 <cmpOgeV4Float>:
; IASM-LABEL:cmpOgeV4Float:

entry:
  %cmp = fcmp oge <4 x float> %a, %b

; ASM:           vcge.f32 q0, q0, q1
; DIS:      6c0: f3000e42
; IASM-NOT:      vcge

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpOltV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpOltV4Float:
; DIS-LABEL:000006d0 <cmpOltV4Float>:
; IASM-LABEL:cmpOltV4Float:

entry:
  %cmp = fcmp olt <4 x float> %a, %b

; ASM:           vcgt.f32 q1, q1, q0
; DIS:      6d0: f3222e40
; IASM-NOT:      vcgt

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpOleV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpOleV4Float:
; DIS-LABEL:000006f0 <cmpOleV4Float>:
; IASM-LABEL:cmpOleV4Float:

entry:
  %cmp = fcmp ole <4 x float> %a, %b

; ASM:           vcge.f32 q1, q1, q0
; DIS:      6f0: f3022e40
; IASM-NOT:      vcge

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpOrdV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpOrdV4Float:
; DIS-LABEL:00000710 <cmpOrdV4Float>:
; IASM-LABEL:cmpOrdV4Float:

entry:
  %cmp = fcmp ord <4 x float> %a, %b

; ASM:           vcge.f32 q2, q0, q1
; ASM-NEXT:      vcgt.f32 q1, q1, q0
; DIS:      710: f3004e42
; DIS-NEXT: 714: f3222e40
; IASM-NOT:      vcge
; IASM-NOT:      vcgt

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpUeqV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpUeqV4Float:
; DIS-LABEL:00000730 <cmpUeqV4Float>:
; IASM-LABEL:cmpUeqV4Float:

entry:
  %cmp = fcmp ueq <4 x float> %a, %b

; ASM:           vcgt.f32 q2, q0, q1
; ASM-NEXT:      vcgt.f32 q1, q1, q0
; ASM-NEXT:      vorr.i32 q2, q2, q1
; ASM-NEXT:      vmvn.i32 q2, q2
; DIS:      730: f3204e42
; DIS-NEXT: 734: f3222e40
; DIS-NEXT: 738: f2244152
; DIS-NEXT: 73c: f3b045c4
; IASM-NOT:      vcgt
; IASM-NOT:      vorr
; IASM-NOT:      vmvn

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpUgtV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpUgtV4Float:
; DIS-LABEL:00000750 <cmpUgtV4Float>:
; IASM-LABEL:cmpUgtV4Float:

entry:
  %cmp = fcmp ugt <4 x float> %a, %b

; ASM:           vcge.f32 q1, q1, q0
; ASM-NEXT:      vmvn.i32 q1, q1
; DIS:      750: f3022e40
; DIS-NEXT: 754: f3b025c2
; IASM-NOT:      vcge
; IASM-NOT:      vmvn

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpUgeV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpUgeV4Float:
; DIS-LABEL:00000770 <cmpUgeV4Float>:
; IASM-LABEL:cmpUgeV4Float:

entry:
  %cmp = fcmp uge <4 x float> %a, %b

; ASM:           vcgt.f32 q1, q1, q0
; ASM-NEXT:      vmvn.i32 q1, q1
; DIS:      770: f3222e40
; DIS-NEXT: 774: f3b025c2
; IASM-NOT:      vcgt
; IASM-NOT:      vmvn

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpUltV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpUltV4Float:
; DIS-LABEL:00000790 <cmpUltV4Float>:
; IASM-LABEL:cmpUltV4Float:

entry:
  %cmp = fcmp ult <4 x float> %a, %b

; ASM:           vcge.f32 q0, q0, q1
; ASM-NEXT:      vmvn.i32 q0, q0
; DIS:      790: f3000e42
; DIS-NEXT: 794: f3b005c0
; IASM-NOT:      vcge
; IASM-NOT:      vmvn

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpUleV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpUleV4Float:
; DIS-LABEL:000007b0 <cmpUleV4Float>:
; IASM-LABEL:cmpUleV4Float:

entry:
  %cmp = fcmp ule <4 x float> %a, %b

; ASM:           vcgt.f32 q0, q0, q1
; ASM-NEXT:      vmvn.i32 q0, q0
; DIS:      7b0: f3200e42
; DIS-NEXT: 7b4: f3b005c0
; IASM-NOT:      vcgt
; IASM-NOT:      vmvn

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}

define internal <4 x i32> @cmpTrueV4Float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:cmpTrueV4Float:
; DIS-LABEL:000007d0 <cmpTrueV4Float>:
; IASM-LABEL:cmpTrueV4Float:

entry:
  %cmp = fcmp true <4 x float> %a, %b

; ASM:           vmov.i32 q0, #1
; DIS:      7d0: f2800051
; IASM-NOT:      vmov

  %zext = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %zext
}
