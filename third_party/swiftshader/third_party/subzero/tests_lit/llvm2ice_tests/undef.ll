; This test checks that undef values are represented as zero.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 -mattr=sse4.1 \
; RUN:   | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 -mattr=sse4.1 \
; RUN:   | FileCheck %s

define internal i32 @undef_i32() {
entry:
  ret i32 undef
; CHECK-LABEL: undef_i32
; CHECK: mov eax,0x0
}

define internal i64 @undef_i64() {
entry:
  ret i64 undef
; CHECK-LABEL: undef_i64
; CHECK-DAG: mov eax,0x0
; CHECK-DAG: mov edx,0x0
; CHECK: ret
}

define internal i32 @trunc_undef_i64() {
entry:
  %ret = trunc i64 undef to i32
  ret i32 %ret
; CHECK-LABEL: trunc_undef_i64
; CHECK: mov eax,0x0
; CHECK: ret
}

define internal float @undef_float() {
entry:
  ret float undef
; CHECK-LABEL: undef_float
; CHECK: xorps [[REG:xmm.]],[[REG]]
; CHECK: fld
}

define internal double @undef_double() {
entry:
  ret double undef
; CHECK-LABEL: undef_double
; CHECK: xorpd [[REG:xmm.]],[[REG]]
; CHECK: fld
}

define internal <4 x i1> @undef_v4i1() {
entry:
  ret <4 x i1> undef
; CHECK-LABEL: undef_v4i1
; CHECK: pxor
}

define internal <8 x i1> @undef_v8i1() {
entry:
  ret <8 x i1> undef
; CHECK-LABEL: undef_v8i1
; CHECK: pxor
}

define internal <16 x i1> @undef_v16i1() {
entry:
  ret <16 x i1> undef
; CHECK-LABEL: undef_v16i1
; CHECK: pxor
}

define internal <16 x i8> @undef_v16i8() {
entry:
  ret <16 x i8> undef
; CHECK-LABEL: undef_v16i8
; CHECK: pxor
}

define internal <8 x i16> @undef_v8i16() {
entry:
  ret <8 x i16> undef
; CHECK-LABEL: undef_v8i16
; CHECK: pxor
}

define internal <4 x i32> @undef_v4i32() {
entry:
  ret <4 x i32> undef
; CHECK-LABEL: undef_v4i32
; CHECK: pxor
}

define internal <4 x float> @undef_v4f32() {
entry:
  ret <4 x float> undef
; CHECK-LABEL: undef_v4f32
; CHECK: pxor
}

define internal <4 x i32> @vector_arith(<4 x i32> %arg) {
entry:
  %val = add <4 x i32> undef, %arg
  ret <4 x i32> %val
; CHECK-LABEL: vector_arith
; CHECK: pxor
}

define internal <4 x float> @vector_bitcast() {
entry:
  %val = bitcast <4 x i32> undef to <4 x float>
  ret <4 x float> %val
; CHECK-LABEL: vector_bitcast
; CHECK: pxor
}

define internal <4 x i32> @vector_sext() {
entry:
  %val = sext <4 x i1> undef to <4 x i32>
  ret <4 x i32> %val
; CHECK-LABEL: vector_sext
; CHECK: pxor
}

define internal <4 x i32> @vector_zext() {
entry:
  %val = zext <4 x i1> undef to <4 x i32>
  ret <4 x i32> %val
; CHECK-LABEL: vector_zext
; CHECK: pxor
}

define internal <4 x i1> @vector_trunc() {
entry:
  %val = trunc <4 x i32> undef to <4 x i1>
  ret <4 x i1> %val
; CHECK-LABEL: vector_trunc
; CHECK: pxor
}

define internal <4 x i1> @vector_icmp(<4 x i32> %arg) {
entry:
  %val = icmp eq <4 x i32> undef, %arg
  ret <4 x i1> %val
; CHECK-LABEL: vector_icmp
; CHECK: pxor
}

define internal <4 x i1> @vector_fcmp(<4 x float> %arg) {
entry:
  %val = fcmp ueq <4 x float> undef, %arg
  ret <4 x i1> %val
; CHECK-LABEL: vector_fcmp
; CHECK: pxor
}

define internal <4 x i32> @vector_fptosi() {
entry:
  %val = fptosi <4 x float> undef to <4 x i32>
  ret <4 x i32> %val
; CHECK-LABEL: vector_fptosi
; CHECK: pxor
}

define internal <4 x i32> @vector_fptoui() {
entry:
  %val = fptoui <4 x float> undef to <4 x i32>
  ret <4 x i32> %val
; CHECK-LABEL: vector_fptoui
; CHECK: pxor
}

define internal <4 x float> @vector_sitofp() {
entry:
  %val = sitofp <4 x i32> undef to <4 x float>
  ret <4 x float> %val
; CHECK-LABEL: vector_sitofp
; CHECK: pxor
}

define internal <4 x float> @vector_uitofp() {
entry:
  %val = uitofp <4 x i32> undef to <4 x float>
  ret <4 x float> %val
; CHECK-LABEL: vector_uitofp
; CHECK: pxor
}

define internal <4 x float> @vector_insertelement_arg1() {
entry:
  %val = insertelement <4 x float> undef, float 1.0, i32 0
  ret <4 x float> %val
; CHECK-LABEL: vector_insertelement_arg1
; CHECK: pxor
}

define internal <4 x float> @vector_insertelement_arg2(<4 x float> %arg) {
entry:
  %val = insertelement <4 x float> %arg, float undef, i32 0
  ret <4 x float> %val
; CHECK-LABEL: vector_insertelement_arg2
; CHECK: xorps [[REG:xmm.]],[[REG]]
; CHECK: {{movss|insertps}} {{.*}},[[REG]]
}

define internal float @vector_extractelement_v4f32_index_0() {
entry:
  %val = extractelement <4 x float> undef, i32 0
  ret float %val
; CHECK-LABEL: vector_extractelement_v4f32_index_0
; CHECK: pxor
}

define internal float @vector_extractelement_v4f32_index_1() {
entry:
  %val = extractelement <4 x float> undef, i32 1
  ret float %val
; CHECK-LABEL: vector_extractelement_v4f32_index_1
; CHECK: pxor
}

define internal i32 @vector_extractelement_v16i1_index_7() {
entry:
  %val.trunc = extractelement <16 x i1> undef, i32 7
  %val = sext i1 %val.trunc to i32
  ret i32 %val
; CHECK-LABEL: vector_extractelement_v16i1_index_7
; CHECK: pxor
}

define internal <4 x i32> @vector_select_v4i32_cond(<4 x i32> %a,
                                                    <4 x i32> %b) {
entry:
  %val = select <4 x i1> undef, <4 x i32> %a, <4 x i32> %b
  ret <4 x i32> %val
; CHECK-LABEL: vector_select_v4i32_cond
; CHECK: pxor
}

define internal <4 x i32> @vector_select_v4i32_arg1(<4 x i1> %cond,
                                                    <4 x i32> %b) {
entry:
  %val = select <4 x i1> %cond, <4 x i32> undef, <4 x i32> %b
  ret <4 x i32> %val
; CHECK-LABEL: vector_select_v4i32_arg1
; CHECK: pxor
}

define internal <4 x i32> @vector_select_v4i32_arg2(<4 x i1> %cond,
                                                    <4 x i32> %a) {
entry:
  %val = select <4 x i1> %cond, <4 x i32> %a, <4 x i32> undef
  ret <4 x i32> %val
; CHECK-LABEL: vector_select_v4i32_arg2
; CHECK: pxor
}

define internal <4 x i1> @vector_select_v4i1_cond(<4 x i1> %a,
                                                  <4 x i1> %b) {
entry:
  %val = select <4 x i1> undef, <4 x i1> %a, <4 x i1> %b
  ret <4 x i1> %val
; CHECK-LABEL: vector_select_v4i1_cond
; CHECK: pxor
}

define internal <4 x i1> @vector_select_v4i1_arg1(<4 x i1> %cond,
                                                  <4 x i1> %b) {
entry:
  %val = select <4 x i1> %cond, <4 x i1> undef, <4 x i1> %b
  ret <4 x i1> %val
; CHECK-LABEL: vector_select_v4i1_arg1
; CHECK: pxor
}

define internal <4 x i1> @vector_select_v4i1_arg2(<4 x i1> %cond,
                                                  <4 x i1> %a) {
entry:
  %val = select <4 x i1> %cond, <4 x i1> %a, <4 x i1> undef
  ret <4 x i1> %val
; CHECK-LABEL: vector_select_v4i1_arg2
; CHECK: pxor
}

define internal <4 x float> @vector_select_v4f32_cond(<4 x float> %a,
                                                      <4 x float> %b) {
entry:
  %val = select <4 x i1> undef, <4 x float> %a, <4 x float> %b
  ret <4 x float> %val
; CHECK-LABEL: vector_select_v4f32_cond
; CHECK: pxor
}

define internal <4 x float> @vector_select_v4f32_arg1(<4 x i1> %cond,
                                                      <4 x float> %b) {
entry:
  %val = select <4 x i1> %cond, <4 x float> undef, <4 x float> %b
  ret <4 x float> %val
; CHECK-LABEL: vector_select_v4f32_arg1
; CHECK: pxor
}

define internal <4 x float> @vector_select_v4f32_arg2(<4 x i1> %cond,
                                                      <4 x float> %a) {
entry:
  %val = select <4 x i1> %cond, <4 x float> %a, <4 x float> undef
  ret <4 x float> %val
; CHECK-LABEL: vector_select_v4f32_arg2
; CHECK: pxor
}
