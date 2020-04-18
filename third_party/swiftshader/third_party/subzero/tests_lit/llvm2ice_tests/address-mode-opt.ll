; This file checks support for address mode optimization.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 -mattr=sse4.1 \
; RUN:   -allow-externally-defined-symbols | FileCheck --check-prefix=SSE41 %s

define internal float @load_arg_plus_200000(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = add i32 %arg.int, 200000
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: load_arg_plus_200000
; CHECK: movss xmm0,DWORD PTR [eax+0x30d40]
}

define internal float @load_200000_plus_arg(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = add i32 200000, %arg.int
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: load_200000_plus_arg
; CHECK: movss xmm0,DWORD PTR [eax+0x30d40]
}

define internal float @load_arg_minus_200000(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = sub i32 %arg.int, 200000
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: load_arg_minus_200000
; CHECK: movss xmm0,DWORD PTR [eax-0x30d40]
}

define internal float @load_200000_minus_arg(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr.int = sub i32 200000, %arg.int
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: load_200000_minus_arg
; CHECK: movss xmm0,DWORD PTR [e{{..}}]
}

define internal <8 x i16> @load_mul_v8i16_mem(<8 x i16> %arg0, i32 %arg1_iptr) {
entry:
  %addr_sub = sub i32 %arg1_iptr, 200000
  %addr_ptr = inttoptr i32 %addr_sub to <8 x i16>*
  %arg1 = load <8 x i16>, <8 x i16>* %addr_ptr, align 2
  %res_vec = mul <8 x i16> %arg0, %arg1
  ret <8 x i16> %res_vec
; Address mode optimization is generally unsafe for SSE vector instructions.
; CHECK-LABEL: load_mul_v8i16_mem
; CHECK-NOT: pmullw xmm{{.*}},XMMWORD PTR [e{{..}}-0x30d40]
}

define internal <4 x i32> @load_mul_v4i32_mem(<4 x i32> %arg0, i32 %arg1_iptr) {
entry:
  %addr_sub = sub i32 %arg1_iptr, 200000
  %addr_ptr = inttoptr i32 %addr_sub to <4 x i32>*
  %arg1 = load <4 x i32>, <4 x i32>* %addr_ptr, align 4
  %res = mul <4 x i32> %arg0, %arg1
  ret <4 x i32> %res
; Address mode optimization is generally unsafe for SSE vector instructions.
; CHECK-LABEL: load_mul_v4i32_mem
; CHECK-NOT: pmuludq xmm{{.*}},XMMWORD PTR [e{{..}}-0x30d40]
; CHECK: pmuludq
;
; SSE41-LABEL: load_mul_v4i32_mem
; SSE41-NOT: pmulld xmm{{.*}},XMMWORD PTR [e{{..}}-0x30d40]
}

define internal float @address_mode_opt_chaining(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = add i32 12, %arg.int
  %addr2.int = sub i32 %addr1.int, 4
  %addr2.ptr = inttoptr i32 %addr2.int to float*
  %addr2.load = load float, float* %addr2.ptr, align 4
  ret float %addr2.load
; CHECK-LABEL: address_mode_opt_chaining
; CHECK: movss xmm0,DWORD PTR [eax+0x8]
}

define internal float @address_mode_opt_chaining_overflow(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = add i32 2147483640, %arg.int
  %addr2.int = add i32 %addr1.int, 2147483643
  %addr2.ptr = inttoptr i32 %addr2.int to float*
  %addr2.load = load float, float* %addr2.ptr, align 4
  ret float %addr2.load
; CHECK-LABEL: address_mode_opt_chaining_overflow
; CHECK: 0x7ffffff8
; CHECK: movss xmm0,DWORD PTR [{{.*}}+0x7ffffffb]
}

define internal float @address_mode_opt_chaining_overflow_sub(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = sub i32 %arg.int, 2147483640
  %addr2.int = sub i32 %addr1.int, 2147483643
  %addr2.ptr = inttoptr i32 %addr2.int to float*
  %addr2.load = load float, float* %addr2.ptr, align 4
  ret float %addr2.load
; CHECK-LABEL: address_mode_opt_chaining_overflow_sub
; CHECK: 0x7ffffff8
; CHECK: movss xmm0,DWORD PTR [{{.*}}-0x7ffffffb]
}

define internal float @address_mode_opt_chaining_no_overflow(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = sub i32 %arg.int, 2147483640
  %addr2.int = add i32 %addr1.int, 2147483643
  %addr2.ptr = inttoptr i32 %addr2.int to float*
  %addr2.load = load float, float* %addr2.ptr, align 4
  ret float %addr2.load
; CHECK-LABEL: address_mode_opt_chaining_no_overflow
; CHECK: movss xmm0,DWORD PTR [{{.*}}+0x3]
}

define internal float @address_mode_opt_add_pos_min_int(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = add i32 %arg.int, 2147483648
  %addr1.ptr = inttoptr i32 %addr1.int to float*
  %addr1.load = load float, float* %addr1.ptr, align 4
  ret float %addr1.load
; CHECK-LABEL: address_mode_opt_add_pos_min_int
; CHECK: movss xmm0,DWORD PTR [{{.*}}-0x80000000]
}

define internal float @address_mode_opt_sub_min_int(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %addr1.int = sub i32 %arg.int, 2147483648
  %addr1.ptr = inttoptr i32 %addr1.int to float*
  %addr1.load = load float, float* %addr1.ptr, align 4
  ret float %addr1.load
; CHECK-LABEL: address_mode_opt_sub_min_int
; CHECK: movss xmm0,DWORD PTR [{{.*}}-0x80000000]
}

define internal float @load_1_or__2_shl_arg(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %shl1 = shl i32 %arg.int, 2
  %addr.int = or i32 1, %shl1
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: load_1_or__2_shl_arg
; CHECK-NOT: or
; CHECK: movss xmm{{[0-9]+}},DWORD PTR [{{e..}}*4+0x1]
}

define internal float @or_add_boundary_check_1(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %shl1 = shl i32 %arg.int, 2
  %addr.int = or i32 5, %shl1
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: or_add_boundary_check_1
; CHECK: or
; CHECK-NOT: movss xmm{{[0-9]+}},DWORD PTR [{{e..}}*4+0x5]
}

define internal float @or_add_boundary_check_2(float* %arg) {
entry:
  %arg.int = ptrtoint float* %arg to i32
  %shl1 = shl i32 %arg.int, 2
  %addr.int = or i32 -1, %shl1
  %addr.ptr = inttoptr i32 %addr.int to float*
  %addr.load = load float, float* %addr.ptr, align 4
  ret float %addr.load
; CHECK-LABEL: or_add_boundary_check_2
; CHECK: or
; CHECK-NOT: movss xmm{{[0-9]+}},DWORD PTR [{{e..}}*4+0xFFFF]
}

define internal void @invert_icmp(i32* %arg1, i32* %arg2) {
entry:
  %addr.other = load i32, i32* %arg2, align 1
  br label %next
next:
  %addr.load = load i32, i32* %arg1, align 1
  %cond = icmp slt i32 %addr.load, %addr.other
  br i1 %cond, label %if.then, label %if.else
if.then:
  ret void
if.else:
  ret void
; CHECK-LABEL: invert_icmp
; CHECK: cmp {{e..}},DWORD PTR [{{e..}}]
; CHECK: jle
}