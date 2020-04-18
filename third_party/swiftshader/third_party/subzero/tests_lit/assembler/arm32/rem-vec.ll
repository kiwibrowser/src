; Show that we know how to translate vector urem, srem and frem.

; NOTE: We use -O2 to get rid of memory stores.

; REQUIRES: allow_dump

; Compile using standalone assembler.
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -O2 -mattr=hwdiv-arm \
; RUN:   | FileCheck %s --check-prefix=ASM

define internal <4 x i32> @Urem4i32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:Urem4i32:

entry:
; ASM-NEXT:.LUrem4i32$entry:

  %v = urem <4 x i32> %a, %b

; ASM-LABEL:.LUrem4i32$local$__0:
; ASM-NEXT: udiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d4[0], r2

; ASM-LABEL:.LUrem4i32$local$__1:
; ASM-NEXT: udiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d4[1], r2

; ASM-LABEL:.LUrem4i32$local$__2:
; ASM-NEXT: udiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d5[0], r2

; ASM-LABEL:.LUrem4i32$local$__3:
; ASM-NEXT: udiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d5[1], r2

  ret <4 x i32> %v
}

define internal <4 x i32> @Srem4i32(<4 x i32> %a, <4 x i32> %b) {
; ASM-LABEL:Srem4i32:

entry:
; ASM-NEXT:.LSrem4i32$entry:

  %v = srem <4 x i32> %a, %b

; ASM-LABEL:.LSrem4i32$local$__0:
; ASM-NEXT: sdiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d4[0], r2

; ASM-LABEL:.LSrem4i32$local$__1:
; ASM-NEXT: sdiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d4[1], r2

; ASM-LABEL:.LSrem4i32$local$__2:
; ASM-NEXT: sdiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d5[0], r2

; ASM-LABEL:.LSrem4i32$local$__3:
; ASM-NEXT: sdiv r2, r0, r1
; ASM-NEXT: mls r2, r2, r1, r0
; ASM-NEXT: vmov.32 d5[1], r2

  ret <4 x i32> %v
}

define internal <4 x float> @Frem4float(<4 x float> %a, <4 x float> %b) {
; ASM-LABEL:Frem4float:

entry:
; ASM-NEXT:.LFrem4float$entry:

  %v = frem <4 x float> %a, %b

; ASM:         vmov.f32 s0, s16
; ASM-NEXT: vmov.f32 s1, s20
; ASM-NEXT: bl fmodf

; ASM:  vmov.f32 s0, s17
; ASM-NEXT: vmov.f32 s1, s21
; ASM-NEXT: bl fmodf

; ASM:  vmov.f32 s0, s18
; ASM-NEXT: vmov.f32 s1, s22
; ASM-NEXT: bl fmodf

; ASM:  vmov.f32 s16, s19
; ASM-NEXT: vmov.f32 s20, s23
; ASM: bl fmodf

  ret <4 x float> %v
}
