; This file tests bitcasts of vector type. For most operations, these
; should be lowered to a no-op on -O2.

; RUN: %p2i -i %s --filetype=obj --disassemble --target=x8632 --args -O2 \
; RUN:   | FileCheck --check-prefix=X86-O2 --check-prefix=X86 %s
; RUN: %p2i -i %s --filetype=obj --disassemble --target=x8632 --args -Om1 \
; RUN:   | FileCheck --check-prefix=X86 %s

; RUN: %p2i -i %s --filetype=obj --disassemble --target=x8664 --args -O2 \
; RUN:   | FileCheck --check-prefix=X86-O2 --check-prefix=X86 %s
; RUN: %p2i -i %s --filetype=obj --disassemble --target=x8664 --args -Om1 \
; RUN:   | FileCheck --check-prefix=X86 %s

; RUN: %p2i -i %s --filetype=obj --disassemble --target=arm32 --args -O2 \
; RUN:   | FileCheck --check-prefix=ARM32-O2-O2 --check-prefix=ARM32 %s
; RUN: %p2i -i %s --filetype=obj --disassemble --target=arm32 --args -Om1 \
; RUN:   | FileCheck --check-prefix=ARM32 %s

define internal <16 x i8> @test_bitcast_v16i8_to_v16i8(<16 x i8> %arg) {
entry:
  %res = bitcast <16 x i8> %arg to <16 x i8>
  ret <16 x i8> %res

; X86-O2-LABEL: test_bitcast_v16i8_to_v16i8
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v16i8_to_v16i8
; ARM32-O2-NEXT: bx
}

define internal <8 x i16> @test_bitcast_v16i8_to_v8i16(<16 x i8> %arg) {
entry:
  %res = bitcast <16 x i8> %arg to <8 x i16>
  ret <8 x i16> %res

; X86-O2-LABEL: test_bitcast_v16i8_to_v8i16
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v16i8_to_v8i16
; ARM32-O2-NEXT: bx
}

define internal <4 x i32> @test_bitcast_v16i8_to_v4i32(<16 x i8> %arg) {
entry:
  %res = bitcast <16 x i8> %arg to <4 x i32>
  ret <4 x i32> %res

; X86-O2-LABEL: test_bitcast_v16i8_to_v4i32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v16i8_to_v4i32
; ARM32-O2-NEXT: bx
}

define internal <4 x float> @test_bitcast_v16i8_to_v4f32(<16 x i8> %arg) {
entry:
  %res = bitcast <16 x i8> %arg to <4 x float>
  ret <4 x float> %res

; X86-O2-LABEL: test_bitcast_v16i8_to_v4f32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v16i8_to_v4f32
; ARM32-O2-NEXT: bx
}

define internal <16 x i8> @test_bitcast_v8i16_to_v16i8(<8 x i16> %arg) {
entry:
  %res = bitcast <8 x i16> %arg to <16 x i8>
  ret <16 x i8> %res

; X86-O2-LABEL: test_bitcast_v8i16_to_v16i8
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v8i16_to_v16i8
; ARM32-O2-NEXT: bx
}

define internal <8 x i16> @test_bitcast_v8i16_to_v8i16(<8 x i16> %arg) {
entry:
  %res = bitcast <8 x i16> %arg to <8 x i16>
  ret <8 x i16> %res

; X86-O2-LABEL: test_bitcast_v8i16_to_v8i16
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v8i16_to_v8i16
; ARM32-O2-NEXT: bx
}

define internal <4 x i32> @test_bitcast_v8i16_to_v4i32(<8 x i16> %arg) {
entry:
  %res = bitcast <8 x i16> %arg to <4 x i32>
  ret <4 x i32> %res

; X86-O2-LABEL: test_bitcast_v8i16_to_v4i32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v8i16_to_v4i32
; ARM32-O2-NEXT: bx
}

define internal <4 x float> @test_bitcast_v8i16_to_v4f32(<8 x i16> %arg) {
entry:
  %res = bitcast <8 x i16> %arg to <4 x float>
  ret <4 x float> %res

; X86-O2-LABEL: test_bitcast_v8i16_to_v4f32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v8i16_to_v4f32
; ARM32-O2-NEXT: bx
}

define internal <16 x i8> @test_bitcast_v4i32_to_v16i8(<4 x i32> %arg) {
entry:
  %res = bitcast <4 x i32> %arg to <16 x i8>
  ret <16 x i8> %res

; X86-O2-LABEL: test_bitcast_v4i32_to_v16i8
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4i32_to_v16i8
; ARM32-O2-NEXT: bx
}

define internal <8 x i16> @test_bitcast_v4i32_to_v8i16(<4 x i32> %arg) {
entry:
  %res = bitcast <4 x i32> %arg to <8 x i16>
  ret <8 x i16> %res

; X86-O2-LABEL: test_bitcast_v4i32_to_v8i16
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4i32_to_v8i16
; ARM32-O2-NEXT: bx
}

define internal <4 x i32> @test_bitcast_v4i32_to_v4i32(<4 x i32> %arg) {
entry:
  %res = bitcast <4 x i32> %arg to <4 x i32>
  ret <4 x i32> %res

; X86-O2-LABEL: test_bitcast_v4i32_to_v4i32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4i32_to_v4i32
; ARM32-O2-NEXT: bx
}

define internal <4 x float> @test_bitcast_v4i32_to_v4f32(<4 x i32> %arg) {
entry:
  %res = bitcast <4 x i32> %arg to <4 x float>
  ret <4 x float> %res

; X86-O2-LABEL: test_bitcast_v4i32_to_v4f32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4i32_to_v4f32
; ARM32-O2-NEXT: bx
}

define internal <16 x i8> @test_bitcast_v4f32_to_v16i8(<4 x float> %arg) {
entry:
  %res = bitcast <4 x float> %arg to <16 x i8>
  ret <16 x i8> %res

; X86-O2-LABEL: test_bitcast_v4f32_to_v16i8
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4f32_to_v16i8
; ARM32-O2-NEXT: bx
}

define internal <8 x i16> @test_bitcast_v4f32_to_v8i16(<4 x float> %arg) {
entry:
  %res = bitcast <4 x float> %arg to <8 x i16>
  ret <8 x i16> %res

; X86-O2-LABEL: test_bitcast_v4f32_to_v8i16
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4f32_to_v8i16
; ARM32-O2-NEXT: bx
}

define internal <4 x i32> @test_bitcast_v4f32_to_v4i32(<4 x float> %arg) {
entry:
  %res = bitcast <4 x float> %arg to <4 x i32>
  ret <4 x i32> %res

; X86-O2-LABEL: test_bitcast_v4f32_to_v4i32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4f32_to_v4i32
; ARM32-O2-NEXT: bx
}

define internal <4 x float> @test_bitcast_v4f32_to_v4f32(<4 x float> %arg) {
entry:
  %res = bitcast <4 x float> %arg to <4 x float>
  ret <4 x float> %res

; X86-O2-LABEL: test_bitcast_v4f32_to_v4f32
; X86-O2-NEXT: ret

; ARM32-O2-LABEL: test_bitcast_v4f32_to_v4f32
; ARM32-O2-NEXT: bx
}

define internal i32 @test_bitcast_v8i1_to_i8(<8 x i1> %arg) {
entry:
  %res = bitcast <8 x i1> %arg to i8
  %res.i32 = zext i8 %res to i32
  ret i32 %res.i32

; X86-LABEL: test_bitcast_v8i1_to_i8
; X86: call {{.*}} R_{{.*}} __Sz_bitcast_8xi1_i8

; ARM32-LABEL: test_bitcast_v8i1_to_i8
; ARM32: bl {{.*}} __Sz_bitcast_8xi1_i8
}

define internal i32 @test_bitcast_v16i1_to_i16(<16 x i1> %arg) {
entry:
  %res = bitcast <16 x i1> %arg to i16
  %res.i32 = zext i16 %res to i32
  ret i32 %res.i32

; X86-LABEL: test_bitcast_v16i1_to_i16
; X86: call {{.*}} __Sz_bitcast_16xi1_i16

; ARM32-LABEL: test_bitcast_v16i1_to_i16
; ARM32: bl {{.*}} __Sz_bitcast_16xi1_i16
}

define internal <8 x i1> @test_bitcast_i8_to_v8i1(i32 %arg) {
entry:
  %arg.trunc = trunc i32 %arg to i8
  %res = bitcast i8 %arg.trunc to <8 x i1>
  ret <8 x i1> %res

; X86-LABEL: test_bitcast_i8_to_v8i1
; X86: call {{.*}} R_{{.*}} __Sz_bitcast_i8_8xi1

; ARM32-LABEL: test_bitcast_i8_to_v8i1
; ARM32: bl {{.*}} __Sz_bitcast_i8_8xi1
}

define internal <16 x i1> @test_bitcast_i16_to_v16i1(i32 %arg) {
entry:
  %arg.trunc = trunc i32 %arg to i16
  %res = bitcast i16 %arg.trunc to <16 x i1>
  ret <16 x i1> %res

; X86-LABEL: test_bitcast_i16_to_v16i1
; X86: call {{.*}} R_{{.*}} __Sz_bitcast_i16_16xi1

; ARM32-LABEL: test_bitcast_i16_to_v16i1
; ARM32: bl {{.*}} __Sz_bitcast_i16_16xi1
}
