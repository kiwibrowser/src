; Tests desired and undesired folding of load instructions into cast
; instructions.  The folding is only done when liveness analysis is performed,
; so only O2 is tested.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 | FileCheck %s

; Not testing trunc, or 32-bit bitcast, because the lowered code uses pretty
; much the same mov instructions regardless of whether folding is done.

define internal i32 @zext_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i8*
  %load = load i8, i8* %addr, align 1
  %result = zext i8 %load to i32
  ret i32 %result
}
; CHECK-LABEL: zext_fold
; CHECK: movzx {{.*}},BYTE PTR [{{.*}}+0xc8]

define internal i32 @zext_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i8*
  %load = load i8, i8* %addr, align 1
  %tmp1 = zext i8 %load to i32
  %tmp2 = zext i8 %load to i32
  %result = add i32 %tmp1, %tmp2
  ret i32 %result
}
; Test that load folding does not happen.
; CHECK-LABEL: zext_nofold
; CHECK-NOT: movzx {{.*}},BYTE PTR [{{.*}}+0xc8]

define internal i32 @sext_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i8*
  %load = load i8, i8* %addr, align 1
  %result = sext i8 %load to i32
  ret i32 %result
}
; CHECK-LABEL: sext_fold
; CHECK: movsx {{.*}},BYTE PTR [{{.*}}+0xc8]

define internal i32 @sext_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i8*
  %load = load i8, i8* %addr, align 1
  %tmp1 = sext i8 %load to i32
  %tmp2 = sext i8 %load to i32
  %result = add i32 %tmp1, %tmp2
  ret i32 %result
}
; Test that load folding does not happen.
; CHECK-LABEL: sext_nofold
; CHECK-NOT: movsx {{.*}},BYTE PTR [{{.*}}+0xc8]

define internal float @fptrunc_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %result = fptrunc double %load to float
  ret float %result
}
; CHECK-LABEL: fptrunc_fold
; CHECK: cvtsd2ss {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal float @fptrunc_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %tmp1 = fptrunc double %load to float
  %tmp2 = fptrunc double %load to float
  %result = fadd float %tmp1, %tmp2
  ret float %result
}
; Test that load folding does not happen.
; CHECK-LABEL: fptrunc_nofold
; CHECK-NOT: cvtsd2ss {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal double @fpext_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to float*
  %load = load float, float* %addr, align 4
  %result = fpext float %load to double
  ret double %result
}
; CHECK-LABEL: fpext_fold
; CHECK: cvtss2sd {{.*}},DWORD PTR [{{.*}}+0xc8]

define internal double @fpext_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to float*
  %load = load float, float* %addr, align 4
  %tmp1 = fpext float %load to double
  %tmp2 = fpext float %load to double
  %result = fadd double %tmp1, %tmp2
  ret double %result
}
; Test that load folding does not happen.
; CHECK-LABEL: fpext_nofold
; CHECK-NOT: cvtss2sd {{.*}},DWORD PTR [{{.*}}+0xc8]

define internal i32 @fptoui_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %result = fptoui double %load to i16
  %result2 = zext i16 %result to i32
  ret i32 %result2
}
; CHECK-LABEL: fptoui_fold
; CHECK: cvttsd2si {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal i32 @fptoui_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %tmp1 = fptoui double %load to i16
  %tmp2 = fptoui double %load to i16
  %result = add i16 %tmp1, %tmp2
  %result2 = zext i16 %result to i32
  ret i32 %result2
}
; Test that load folding does not happen.
; CHECK-LABEL: fptoui_nofold
; CHECK-NOT: cvttsd2si {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal i32 @fptosi_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %result = fptosi double %load to i16
  %result2 = zext i16 %result to i32
  ret i32 %result2
}
; CHECK-LABEL: fptosi_fold
; CHECK: cvttsd2si {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal i32 @fptosi_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %tmp1 = fptosi double %load to i16
  %tmp2 = fptosi double %load to i16
  %result = add i16 %tmp1, %tmp2
  %result2 = zext i16 %result to i32
  ret i32 %result2
}
; Test that load folding does not happen.
; CHECK-LABEL: fptosi_nofold
; CHECK-NOT: cvttsd2si {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal double @uitofp_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i16*
  %load = load i16, i16* %addr, align 1
  %result = uitofp i16 %load to double
  ret double %result
}
; CHECK-LABEL: uitofp_fold
; CHECK: movzx {{.*}},WORD PTR [{{.*}}+0xc8]

define internal double @uitofp_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i16*
  %load = load i16, i16* %addr, align 1
  %tmp1 = uitofp i16 %load to double
  %tmp2 = uitofp i16 %load to double
  %result = fadd double %tmp1, %tmp2
  ret double %result
}
; Test that load folding does not happen.
; CHECK-LABEL: uitofp_nofold
; CHECK-NOT: movzx {{.*}},WORD PTR [{{.*}}+0xc8]

define internal double @sitofp_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i16*
  %load = load i16, i16* %addr, align 1
  %result = sitofp i16 %load to double
  ret double %result
}
; CHECK-LABEL: sitofp_fold
; CHECK: movsx {{.*}},WORD PTR [{{.*}}+0xc8]

define internal double @sitofp_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i16*
  %load = load i16, i16* %addr, align 1
  %tmp1 = sitofp i16 %load to double
  %tmp2 = sitofp i16 %load to double
  %result = fadd double %tmp1, %tmp2
  ret double %result
}
; Test that load folding does not happen.
; CHECK-LABEL: sitofp_nofold
; CHECK-NOT: movsx {{.*}},WORD PTR [{{.*}}+0xc8]

define internal double @bitcast_i64_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i64*
  %load = load i64, i64* %addr, align 1
  %result = bitcast i64 %load to double
  ret double %result
}
; CHECK-LABEL: bitcast_i64_fold
; CHECK: movq {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal double @bitcast_i64_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to i64*
  %load = load i64, i64* %addr, align 1
  %tmp1 = bitcast i64 %load to double
  %tmp2 = bitcast i64 %load to double
  %result = fadd double %tmp1, %tmp2
  ret double %result
}
; Test that load folding does not happen.
; CHECK-LABEL: bitcast_i64_nofold
; CHECK-NOT: movq {{.*}},QWORD PTR [{{.*}}+0xc8]

define internal i64 @bitcast_double_fold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %result = bitcast double %load to i64
  ret i64 %result
}
; CHECK-LABEL: bitcast_double_fold
; CHECK-NOT: QWORD PTR
; CHECK: mov {{.*}},DWORD PTR [{{.*}}+0xc8]
; CHECK: mov {{.*}},DWORD PTR [{{.*}}+0xcc]
; CHECK-NOT: QWORD PTR

define internal i64 @bitcast_double_nofold(i32 %arg) {
entry:
  %ptr = add i32 %arg, 200
  %addr = inttoptr i32 %ptr to double*
  %load = load double, double* %addr, align 8
  %tmp1 = bitcast double %load to i64
  %tmp2 = bitcast double %load to i64
  %result = add i64 %tmp1, %tmp2
  ret i64 %result
}
; Test that load folding does not happen.
; CHECK-LABEL: bitcast_double_nofold
; CHECK: QWORD PTR
; CHECK: QWORD PTR
