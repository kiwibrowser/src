; Show that we know how to translate insertelement and extractelement.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=ASM

; Show bytes in assembled standalone code.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=DIS

; Compile using integrated assembler.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=IASM

; Show bytes in assembled integrated code.
; RUN: %p2i --filetype=iasm -i %s --target=arm32 --assemble --disassemble \
; RUN:   --args -Om1 \
; RUN:   | FileCheck %s --check-prefix=DIS

define internal i32 @extract1_v4i32(<4 x i32> %src) {
; ASM-LABEL: extract1_v4i32:
; DIS-LABEL: 00000000 <extract1_v4i32>:
; IASM-LABEL: extract1_v4i32:

  %1 = extractelement <4 x i32> %src, i32 1

; ASM: vmov.32  r0, d0[1]
; DIS:   10:       ee300b10
; IASM-NOT: vmov.32  r0, d0[1]
  ret i32 %1
}

define internal i32 @extract2_v4i32(<4 x i32> %src) {
; ASM-LABEL: extract2_v4i32:
; DIS-LABEL: 00000030 <extract2_v4i32>:
; IASM-LABEL: extract2_v4i32:

  %1 = extractelement <4 x i32> %src, i32 2

; ASM: vmov.32  r0, d1[0]
; DIS:   40:       ee110b10
; IASM-NOT: vmov.32  r0, d1[0]

  ret i32 %1
}

define internal i32 @extract3_v8i16(<8 x i16> %src) {
; ASM-LABEL: extract3_v8i16:
; DIS-LABEL: 00000060 <extract3_v8i16>:
; IASM-LABEL: extract3_v8i16:

  %1 = extractelement <8 x i16> %src, i32 3

; ASM: vmov.s16 r0, d0[3]
; DIS:   70:       ee300b70
; IASM-NOT: vmov.s16 r0, d0[3]

  %2 = sext i16 %1 to i32
  ret i32 %2
}

define internal i32 @extract4_v8i16(<8 x i16> %src) {
; ASM-LABEL: extract4_v8i16:
; DIS-LABEL: 00000090 <extract4_v8i16>:
; IASM-LABEL: extract4_v8i16:

  %1 = extractelement <8 x i16> %src, i32 4

; ASM: vmov.s16 r0, d1[0]
; DIS:   a0:       ee110b30
; IASM-NOT: vmov.s16 r0, d1[0]

  %2 = sext i16 %1 to i32
  ret i32 %2
}

define internal i32 @extract7_v4i8(<16 x i8> %src) {
; ASM-LABEL: extract7_v4i8:
; DIS-LABEL: 000000c0 <extract7_v4i8>:
; IASM-LABEL: extract7_v4i8:

  %1 = extractelement <16 x i8> %src, i32 7

; ASM: vmov.s8  r0, d0[7]
; DIS:   d0:       ee700b70
; IASM-NOT: vmov.s8  r0, d0[7]

  %2 = sext i8 %1 to i32
  ret i32 %2
}

define internal i32 @extract8_v16i8(<16 x i8> %src) {
; ASM-LABEL: extract8_v16i8:
; DIS-LABEL: 000000f0 <extract8_v16i8>:
; IASM-LABEL: extract8_v16i8:

  %1 = extractelement <16 x i8> %src, i32 8

; ASM: vmov.s8  r0, d1[0]
; DIS:   100:       ee510b10
; IASM-NOT: vmov.s8  r0, d1[0]

  %2 = sext i8 %1 to i32
  ret i32 %2
}

define internal float @extract1_v4float(<4 x float> %src) {
; ASM-LABEL: extract1_v4float:
; DIS-LABEL: 00000120 <extract1_v4float>:
; IASM-LABEL: extract1_v4float:

  %1 = extractelement <4 x float> %src, i32 1

; ASM: vmov.f32 s0, s1
; DIS:   130:       eeb00a60
; IASM-NOT: vmov.f32 s0, s1

  ret float %1
}

define internal float @extract2_v4float(<4 x float> %src) {
; ASM-LABEL: extract2_v4float:
; DIS-LABEL: 00000150 <extract2_v4float>:
; IASM-LABEL: extract2_v4float:

  %1 = extractelement <4 x float> %src, i32 2

; ASM: vmov.f32 s0, s2
; DIS:   160:       eeb00a41
; IASM-NOT: vmov.f32 s0, s2

  ret float %1
}

define internal <4 x i32> @insert1_v4i32(<4 x i32> %src, i32 %s) {
; ASM-LABEL: insert1_v4i32:
; DIS-LABEL: 00000180 <insert1_v4i32>:
; IASM-LABEL: insert1_v4i32:

  %1 = insertelement <4 x i32> %src, i32 %s, i32 1

; ASM: vmov.32  d0[1], r0
; DIS:   198:       ee200b10
; IASM-NOT: vmov.32  d0[1], r0

  ret <4 x i32> %1
}

define internal <4 x i32> @insert2_v4i32(<4 x i32> %src, i32 %s) {
; ASM-LABEL: insert2_v4i32:
; DIS-LABEL: 000001b0 <insert2_v4i32>:
; IASM-LABEL: insert2_v4i32:

  %1 = insertelement <4 x i32> %src, i32 %s, i32 2

; ASM: vmov.32  d1[0], r0
; DIS:   1c8:       ee010b10
; IASM-NOT: vmov.32  d1[0], r0

  ret <4 x i32> %1
}

define internal <8 x i16> @insert3_v8i16(<8 x i16> %src, i32 %s) {
; ASM-LABEL: insert3_v8i16:
; DIS-LABEL: 000001e0 <insert3_v8i16>:
; IASM-LABEL: insert3_v8i16:

  %s2 = trunc i32 %s to i16
  %1 = insertelement <8 x i16> %src, i16 %s2, i32 3

; ASM: vmov.16  d0[3], r0
; DIS:   200:       ee200b70
; IASM-NOT: vmov.16  d0[3], r0

  ret <8 x i16> %1
}

define internal <8 x i16> @insert4_v8i16(<8 x i16> %src, i32 %s) {
; ASM-LABEL: insert4_v8i16:
; DIS-LABEL: 00000220 <insert4_v8i16>:
; IASM-LABEL: insert4_v8i16:

  %s2 = trunc i32 %s to i16
  %1 = insertelement <8 x i16> %src, i16 %s2, i32 4

; ASM: vmov.16  d1[0], r0
; DIS:   240:       ee010b30
; IASM-NOT: vmov.16  d1[0], r0

  ret <8 x i16> %1
}

define internal <16 x i8> @insert7_v4i8(<16 x i8> %src, i32 %s) {
; ASM-LABEL: insert7_v4i8:
; DIS-LABEL: 00000260 <insert7_v4i8>:
; IASM-LABEL: insert7_v4i8:

  %s2 = trunc i32 %s to i8
  %1 = insertelement <16 x i8> %src, i8 %s2, i32 7

; ASM: vmov.8   d0[7], r0
; DIS:   280:       ee600b70
; IASM-NOT: vmov.8   d0[7], r0

  ret <16 x i8> %1
}

define internal <16 x i8> @insert8_v16i8(<16 x i8> %src, i32 %s) {
; ASM-LABEL: insert8_v16i8:
; DIS-LABEL: 000002a0 <insert8_v16i8>:
; IASM-LABEL: insert8_v16i8:

  %s2 = trunc i32 %s to i8
  %1 = insertelement <16 x i8> %src, i8 %s2, i32 8

; ASM: vmov.8   d1[0], r0
; DIS:   2c0:       ee410b10
; IASM-NOT: vmov.8   d1[0], r0

  ret <16 x i8> %1
}

define internal <4 x float> @insert1_v4float(<4 x float> %src, float %s) {
; ASM-LABEL: insert1_v4float:
; DIS-LABEL: 000002e0 <insert1_v4float>:
; IASM-LABEL: insert1_v4float:

  %1 = insertelement <4 x float> %src, float %s, i32 1

; ASM: vmov.f32 s1, s4
; DIS:   2f8:       eef00a42
; IASM-NOT: vmov.f32 s1, s4

  ret <4 x float> %1
}

define internal <4 x float> @insert2_v4float(<4 x float> %src, float %s) {
; ASM-LABEL: insert2_v4float:
; DIS-LABEL: 00000310 <insert2_v4float>:
; IASM-LABEL: insert2_v4float:

  %1 = insertelement <4 x float> %src, float %s, i32 2

; ASM: vmov.f32 s2, s4
; DIS:   328:       eeb01a42
; IASM-NOT: vmov.f32 s2, s4

  ret <4 x float> %1
}
