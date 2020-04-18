; Test if we can read compare instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @IcmpI1(i32 %p1, i32 %p2) {
entry:
  %a1 = trunc i32 %p1 to i1
  %a2 = trunc i32 %p2 to i1
  %veq = icmp eq i1 %a1, %a2
  %vne = icmp ne i1 %a1, %a2
  %vugt = icmp ugt i1 %a1, %a2
  %vuge = icmp uge i1 %a1, %a2
  %vult = icmp ult i1 %a1, %a2
  %vule = icmp ule i1 %a1, %a2
  %vsgt = icmp sgt i1 %a1, %a2
  %vsge = icmp sge i1 %a1, %a2
  %vslt = icmp slt i1 %a1, %a2
  %vsle = icmp sle i1 %a1, %a2
  ret void
}

; CHECK:      define internal void @IcmpI1(i32 %p1, i32 %p2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a1 = trunc i32 %p1 to i1
; CHECK-NEXT:   %a2 = trunc i32 %p2 to i1
; CHECK-NEXT:   %veq = icmp eq i1 %a1, %a2
; CHECK-NEXT:   %vne = icmp ne i1 %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt i1 %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge i1 %a1, %a2
; CHECK-NEXT:   %vult = icmp ult i1 %a1, %a2
; CHECK-NEXT:   %vule = icmp ule i1 %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt i1 %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge i1 %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt i1 %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle i1 %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @IcmpI8(i32 %p1, i32 %p2) {
entry:
  %a1 = trunc i32 %p1 to i8
  %a2 = trunc i32 %p2 to i8
  %veq = icmp eq i8 %a1, %a2
  %vne = icmp ne i8 %a1, %a2
  %vugt = icmp ugt i8 %a1, %a2
  %vuge = icmp uge i8 %a1, %a2
  %vult = icmp ult i8 %a1, %a2
  %vule = icmp ule i8 %a1, %a2
  %vsgt = icmp sgt i8 %a1, %a2
  %vsge = icmp sge i8 %a1, %a2
  %vslt = icmp slt i8 %a1, %a2
  %vsle = icmp sle i8 %a1, %a2
  ret void
}

; CHECK-NEXT: define internal void @IcmpI8(i32 %p1, i32 %p2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a1 = trunc i32 %p1 to i8
; CHECK-NEXT:   %a2 = trunc i32 %p2 to i8
; CHECK-NEXT:   %veq = icmp eq i8 %a1, %a2
; CHECK-NEXT:   %vne = icmp ne i8 %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt i8 %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge i8 %a1, %a2
; CHECK-NEXT:   %vult = icmp ult i8 %a1, %a2
; CHECK-NEXT:   %vule = icmp ule i8 %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt i8 %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge i8 %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt i8 %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle i8 %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @IcmpI16(i32 %p1, i32 %p2) {
entry:
  %a1 = trunc i32 %p1 to i16
  %a2 = trunc i32 %p2 to i16
  %veq = icmp eq i16 %a1, %a2
  %vne = icmp ne i16 %a1, %a2
  %vugt = icmp ugt i16 %a1, %a2
  %vuge = icmp uge i16 %a1, %a2
  %vult = icmp ult i16 %a1, %a2
  %vule = icmp ule i16 %a1, %a2
  %vsgt = icmp sgt i16 %a1, %a2
  %vsge = icmp sge i16 %a1, %a2
  %vslt = icmp slt i16 %a1, %a2
  %vsle = icmp sle i16 %a1, %a2
  ret void
}

; CHECK-NEXT: define internal void @IcmpI16(i32 %p1, i32 %p2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a1 = trunc i32 %p1 to i16
; CHECK-NEXT:   %a2 = trunc i32 %p2 to i16
; CHECK-NEXT:   %veq = icmp eq i16 %a1, %a2
; CHECK-NEXT:   %vne = icmp ne i16 %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt i16 %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge i16 %a1, %a2
; CHECK-NEXT:   %vult = icmp ult i16 %a1, %a2
; CHECK-NEXT:   %vule = icmp ule i16 %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt i16 %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge i16 %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt i16 %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle i16 %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @IcmpI32(i32 %a1, i32 %a2) {
entry:
  %veq = icmp eq i32 %a1, %a2
  %vne = icmp ne i32 %a1, %a2
  %vugt = icmp ugt i32 %a1, %a2
  %vuge = icmp uge i32 %a1, %a2
  %vult = icmp ult i32 %a1, %a2
  %vule = icmp ule i32 %a1, %a2
  %vsgt = icmp sgt i32 %a1, %a2
  %vsge = icmp sge i32 %a1, %a2
  %vslt = icmp slt i32 %a1, %a2
  %vsle = icmp sle i32 %a1, %a2
  ret void
}

; CHECK-NEXT: define internal void @IcmpI32(i32 %a1, i32 %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq i32 %a1, %a2
; CHECK-NEXT:   %vne = icmp ne i32 %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt i32 %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge i32 %a1, %a2
; CHECK-NEXT:   %vult = icmp ult i32 %a1, %a2
; CHECK-NEXT:   %vule = icmp ule i32 %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt i32 %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge i32 %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt i32 %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle i32 %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @IcmpI64(i64 %a1, i64 %a2) {
entry:
  %veq = icmp eq i64 %a1, %a2
  %vne = icmp ne i64 %a1, %a2
  %vugt = icmp ugt i64 %a1, %a2
  %vuge = icmp uge i64 %a1, %a2
  %vult = icmp ult i64 %a1, %a2
  %vule = icmp ule i64 %a1, %a2
  %vsgt = icmp sgt i64 %a1, %a2
  %vsge = icmp sge i64 %a1, %a2
  %vslt = icmp slt i64 %a1, %a2
  %vsle = icmp sle i64 %a1, %a2
  ret void
}

; CHECK-NEXT: define internal void @IcmpI64(i64 %a1, i64 %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq i64 %a1, %a2
; CHECK-NEXT:   %vne = icmp ne i64 %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt i64 %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge i64 %a1, %a2
; CHECK-NEXT:   %vult = icmp ult i64 %a1, %a2
; CHECK-NEXT:   %vule = icmp ule i64 %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt i64 %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge i64 %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt i64 %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle i64 %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal <4 x i1> @IcmpV4xI1(<4 x i1> %a1, <4 x i1> %a2) {
entry:
  %veq = icmp eq <4 x i1> %a1, %a2
  %vne = icmp ne <4 x i1> %a1, %a2
  %vugt = icmp ugt <4 x i1> %a1, %a2
  %vuge = icmp uge <4 x i1> %a1, %a2
  %vult = icmp ult <4 x i1> %a1, %a2
  %vule = icmp ule <4 x i1> %a1, %a2
  %vsgt = icmp sgt <4 x i1> %a1, %a2
  %vsge = icmp sge <4 x i1> %a1, %a2
  %vslt = icmp slt <4 x i1> %a1, %a2
  %vsle = icmp sle <4 x i1> %a1, %a2
  ret <4 x i1> %veq
}

; CHECK-NEXT: define internal <4 x i1> @IcmpV4xI1(<4 x i1> %a1, <4 x i1> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq <4 x i1> %a1, %a2
; CHECK-NEXT:   %vne = icmp ne <4 x i1> %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt <4 x i1> %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge <4 x i1> %a1, %a2
; CHECK-NEXT:   %vult = icmp ult <4 x i1> %a1, %a2
; CHECK-NEXT:   %vule = icmp ule <4 x i1> %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt <4 x i1> %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge <4 x i1> %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt <4 x i1> %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle <4 x i1> %a1, %a2
; CHECK-NEXT:   ret <4 x i1> %veq
; CHECK-NEXT: }

define internal <8 x i1> @IcmpV8xI1(<8 x i1> %a1, <8 x i1> %a2) {
entry:
  %veq = icmp eq <8 x i1> %a1, %a2
  %vne = icmp ne <8 x i1> %a1, %a2
  %vugt = icmp ugt <8 x i1> %a1, %a2
  %vuge = icmp uge <8 x i1> %a1, %a2
  %vult = icmp ult <8 x i1> %a1, %a2
  %vule = icmp ule <8 x i1> %a1, %a2
  %vsgt = icmp sgt <8 x i1> %a1, %a2
  %vsge = icmp sge <8 x i1> %a1, %a2
  %vslt = icmp slt <8 x i1> %a1, %a2
  %vsle = icmp sle <8 x i1> %a1, %a2
  ret <8 x i1> %veq
}

; CHECK-NEXT: define internal <8 x i1> @IcmpV8xI1(<8 x i1> %a1, <8 x i1> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq <8 x i1> %a1, %a2
; CHECK-NEXT:   %vne = icmp ne <8 x i1> %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt <8 x i1> %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge <8 x i1> %a1, %a2
; CHECK-NEXT:   %vult = icmp ult <8 x i1> %a1, %a2
; CHECK-NEXT:   %vule = icmp ule <8 x i1> %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt <8 x i1> %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge <8 x i1> %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt <8 x i1> %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle <8 x i1> %a1, %a2
; CHECK-NEXT:   ret <8 x i1> %veq
; CHECK-NEXT: }

define internal <16 x i1> @IcmpV16xI1(<16 x i1> %a1, <16 x i1> %a2) {
entry:
  %veq = icmp eq <16 x i1> %a1, %a2
  %vne = icmp ne <16 x i1> %a1, %a2
  %vugt = icmp ugt <16 x i1> %a1, %a2
  %vuge = icmp uge <16 x i1> %a1, %a2
  %vult = icmp ult <16 x i1> %a1, %a2
  %vule = icmp ule <16 x i1> %a1, %a2
  %vsgt = icmp sgt <16 x i1> %a1, %a2
  %vsge = icmp sge <16 x i1> %a1, %a2
  %vslt = icmp slt <16 x i1> %a1, %a2
  %vsle = icmp sle <16 x i1> %a1, %a2
  ret <16 x i1> %veq
}

; CHECK-NEXT: define internal <16 x i1> @IcmpV16xI1(<16 x i1> %a1, <16 x i1> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq <16 x i1> %a1, %a2
; CHECK-NEXT:   %vne = icmp ne <16 x i1> %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt <16 x i1> %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge <16 x i1> %a1, %a2
; CHECK-NEXT:   %vult = icmp ult <16 x i1> %a1, %a2
; CHECK-NEXT:   %vule = icmp ule <16 x i1> %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt <16 x i1> %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge <16 x i1> %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt <16 x i1> %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle <16 x i1> %a1, %a2
; CHECK-NEXT:   ret <16 x i1> %veq
; CHECK-NEXT: }

define internal <16 x i1> @IcmpV16xI8(<16 x i8> %a1, <16 x i8> %a2) {
entry:
  %veq = icmp eq <16 x i8> %a1, %a2
  %vne = icmp ne <16 x i8> %a1, %a2
  %vugt = icmp ugt <16 x i8> %a1, %a2
  %vuge = icmp uge <16 x i8> %a1, %a2
  %vult = icmp ult <16 x i8> %a1, %a2
  %vule = icmp ule <16 x i8> %a1, %a2
  %vsgt = icmp sgt <16 x i8> %a1, %a2
  %vsge = icmp sge <16 x i8> %a1, %a2
  %vslt = icmp slt <16 x i8> %a1, %a2
  %vsle = icmp sle <16 x i8> %a1, %a2
  ret <16 x i1> %veq
}

; CHECK-NEXT: define internal <16 x i1> @IcmpV16xI8(<16 x i8> %a1, <16 x i8> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq <16 x i8> %a1, %a2
; CHECK-NEXT:   %vne = icmp ne <16 x i8> %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt <16 x i8> %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge <16 x i8> %a1, %a2
; CHECK-NEXT:   %vult = icmp ult <16 x i8> %a1, %a2
; CHECK-NEXT:   %vule = icmp ule <16 x i8> %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt <16 x i8> %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge <16 x i8> %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt <16 x i8> %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle <16 x i8> %a1, %a2
; CHECK-NEXT:   ret <16 x i1> %veq
; CHECK-NEXT: }

define internal <8 x i1> @IcmpV8xI16(<8 x i16> %a1, <8 x i16> %a2) {
entry:
  %veq = icmp eq <8 x i16> %a1, %a2
  %vne = icmp ne <8 x i16> %a1, %a2
  %vugt = icmp ugt <8 x i16> %a1, %a2
  %vuge = icmp uge <8 x i16> %a1, %a2
  %vult = icmp ult <8 x i16> %a1, %a2
  %vule = icmp ule <8 x i16> %a1, %a2
  %vsgt = icmp sgt <8 x i16> %a1, %a2
  %vsge = icmp sge <8 x i16> %a1, %a2
  %vslt = icmp slt <8 x i16> %a1, %a2
  %vsle = icmp sle <8 x i16> %a1, %a2
  ret <8 x i1> %veq
}

; CHECK-NEXT: define internal <8 x i1> @IcmpV8xI16(<8 x i16> %a1, <8 x i16> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq <8 x i16> %a1, %a2
; CHECK-NEXT:   %vne = icmp ne <8 x i16> %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt <8 x i16> %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge <8 x i16> %a1, %a2
; CHECK-NEXT:   %vult = icmp ult <8 x i16> %a1, %a2
; CHECK-NEXT:   %vule = icmp ule <8 x i16> %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt <8 x i16> %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge <8 x i16> %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt <8 x i16> %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle <8 x i16> %a1, %a2
; CHECK-NEXT:   ret <8 x i1> %veq
; CHECK-NEXT: }

define internal <4 x i1> @IcmpV4xI32(<4 x i32> %a1, <4 x i32> %a2) {
entry:
  %veq = icmp eq <4 x i32> %a1, %a2
  %vne = icmp ne <4 x i32> %a1, %a2
  %vugt = icmp ugt <4 x i32> %a1, %a2
  %vuge = icmp uge <4 x i32> %a1, %a2
  %vult = icmp ult <4 x i32> %a1, %a2
  %vule = icmp ule <4 x i32> %a1, %a2
  %vsgt = icmp sgt <4 x i32> %a1, %a2
  %vsge = icmp sge <4 x i32> %a1, %a2
  %vslt = icmp slt <4 x i32> %a1, %a2
  %vsle = icmp sle <4 x i32> %a1, %a2
  ret <4 x i1> %veq
}

; CHECK-NEXT: define internal <4 x i1> @IcmpV4xI32(<4 x i32> %a1, <4 x i32> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %veq = icmp eq <4 x i32> %a1, %a2
; CHECK-NEXT:   %vne = icmp ne <4 x i32> %a1, %a2
; CHECK-NEXT:   %vugt = icmp ugt <4 x i32> %a1, %a2
; CHECK-NEXT:   %vuge = icmp uge <4 x i32> %a1, %a2
; CHECK-NEXT:   %vult = icmp ult <4 x i32> %a1, %a2
; CHECK-NEXT:   %vule = icmp ule <4 x i32> %a1, %a2
; CHECK-NEXT:   %vsgt = icmp sgt <4 x i32> %a1, %a2
; CHECK-NEXT:   %vsge = icmp sge <4 x i32> %a1, %a2
; CHECK-NEXT:   %vslt = icmp slt <4 x i32> %a1, %a2
; CHECK-NEXT:   %vsle = icmp sle <4 x i32> %a1, %a2
; CHECK-NEXT:   ret <4 x i1> %veq
; CHECK-NEXT: }

define internal void @FcmpFloat(float %a1, float %a2) {
entry:
  %vfalse = fcmp false float %a1, %a2
  %voeq = fcmp oeq float %a1, %a2
  %vogt = fcmp ogt float %a1, %a2
  %voge = fcmp oge float %a1, %a2
  %volt = fcmp olt float %a1, %a2
  %vole = fcmp ole float %a1, %a2
  %vone = fcmp one float %a1, %a2
  %ord = fcmp ord float %a1, %a2
  %vueq = fcmp ueq float %a1, %a2
  %vugt = fcmp ugt float %a1, %a2
  %vuge = fcmp uge float %a1, %a2
  %vult = fcmp ult float %a1, %a2
  %vule = fcmp ule float %a1, %a2
  %vune = fcmp une float %a1, %a2
  %vuno = fcmp uno float %a1, %a2
  %vtrue = fcmp true float %a1, %a2
  ret void
}

; CHECK-NEXT: define internal void @FcmpFloat(float %a1, float %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vfalse = fcmp false float %a1, %a2
; CHECK-NEXT:   %voeq = fcmp oeq float %a1, %a2
; CHECK-NEXT:   %vogt = fcmp ogt float %a1, %a2
; CHECK-NEXT:   %voge = fcmp oge float %a1, %a2
; CHECK-NEXT:   %volt = fcmp olt float %a1, %a2
; CHECK-NEXT:   %vole = fcmp ole float %a1, %a2
; CHECK-NEXT:   %vone = fcmp one float %a1, %a2
; CHECK-NEXT:   %ord = fcmp ord float %a1, %a2
; CHECK-NEXT:   %vueq = fcmp ueq float %a1, %a2
; CHECK-NEXT:   %vugt = fcmp ugt float %a1, %a2
; CHECK-NEXT:   %vuge = fcmp uge float %a1, %a2
; CHECK-NEXT:   %vult = fcmp ult float %a1, %a2
; CHECK-NEXT:   %vule = fcmp ule float %a1, %a2
; CHECK-NEXT:   %vune = fcmp une float %a1, %a2
; CHECK-NEXT:   %vuno = fcmp uno float %a1, %a2
; CHECK-NEXT:   %vtrue = fcmp true float %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @FcmpDouble(double %a1, double %a2) {
entry:
  %vfalse = fcmp false double %a1, %a2
  %voeq = fcmp oeq double %a1, %a2
  %vogt = fcmp ogt double %a1, %a2
  %voge = fcmp oge double %a1, %a2
  %volt = fcmp olt double %a1, %a2
  %vole = fcmp ole double %a1, %a2
  %vone = fcmp one double %a1, %a2
  %ord = fcmp ord double %a1, %a2
  %vueq = fcmp ueq double %a1, %a2
  %vugt = fcmp ugt double %a1, %a2
  %vuge = fcmp uge double %a1, %a2
  %vult = fcmp ult double %a1, %a2
  %vule = fcmp ule double %a1, %a2
  %vune = fcmp une double %a1, %a2
  %vuno = fcmp uno double %a1, %a2
  %vtrue = fcmp true double %a1, %a2
  ret void
}

; CHECK-NEXT: define internal void @FcmpDouble(double %a1, double %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vfalse = fcmp false double %a1, %a2
; CHECK-NEXT:   %voeq = fcmp oeq double %a1, %a2
; CHECK-NEXT:   %vogt = fcmp ogt double %a1, %a2
; CHECK-NEXT:   %voge = fcmp oge double %a1, %a2
; CHECK-NEXT:   %volt = fcmp olt double %a1, %a2
; CHECK-NEXT:   %vole = fcmp ole double %a1, %a2
; CHECK-NEXT:   %vone = fcmp one double %a1, %a2
; CHECK-NEXT:   %ord = fcmp ord double %a1, %a2
; CHECK-NEXT:   %vueq = fcmp ueq double %a1, %a2
; CHECK-NEXT:   %vugt = fcmp ugt double %a1, %a2
; CHECK-NEXT:   %vuge = fcmp uge double %a1, %a2
; CHECK-NEXT:   %vult = fcmp ult double %a1, %a2
; CHECK-NEXT:   %vule = fcmp ule double %a1, %a2
; CHECK-NEXT:   %vune = fcmp une double %a1, %a2
; CHECK-NEXT:   %vuno = fcmp uno double %a1, %a2
; CHECK-NEXT:   %vtrue = fcmp true double %a1, %a2
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal <4 x i1> @FcmpV4xFloat(<4 x float> %a1, <4 x float> %a2) {
entry:
  %vfalse = fcmp false <4 x float> %a1, %a2
  %voeq = fcmp oeq <4 x float> %a1, %a2
  %vogt = fcmp ogt <4 x float> %a1, %a2
  %voge = fcmp oge <4 x float> %a1, %a2
  %volt = fcmp olt <4 x float> %a1, %a2
  %vole = fcmp ole <4 x float> %a1, %a2
  %vone = fcmp one <4 x float> %a1, %a2
  %ord = fcmp ord <4 x float> %a1, %a2
  %vueq = fcmp ueq <4 x float> %a1, %a2
  %vugt = fcmp ugt <4 x float> %a1, %a2
  %vuge = fcmp uge <4 x float> %a1, %a2
  %vult = fcmp ult <4 x float> %a1, %a2
  %vule = fcmp ule <4 x float> %a1, %a2
  %vune = fcmp une <4 x float> %a1, %a2
  %vuno = fcmp uno <4 x float> %a1, %a2
  %vtrue = fcmp true <4 x float> %a1, %a2
  ret <4 x i1> %voeq
}

; CHECK-NEXT: define internal <4 x i1> @FcmpV4xFloat(<4 x float> %a1, <4 x float> %a2) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %vfalse = fcmp false <4 x float> %a1, %a2
; CHECK-NEXT:   %voeq = fcmp oeq <4 x float> %a1, %a2
; CHECK-NEXT:   %vogt = fcmp ogt <4 x float> %a1, %a2
; CHECK-NEXT:   %voge = fcmp oge <4 x float> %a1, %a2
; CHECK-NEXT:   %volt = fcmp olt <4 x float> %a1, %a2
; CHECK-NEXT:   %vole = fcmp ole <4 x float> %a1, %a2
; CHECK-NEXT:   %vone = fcmp one <4 x float> %a1, %a2
; CHECK-NEXT:   %ord = fcmp ord <4 x float> %a1, %a2
; CHECK-NEXT:   %vueq = fcmp ueq <4 x float> %a1, %a2
; CHECK-NEXT:   %vugt = fcmp ugt <4 x float> %a1, %a2
; CHECK-NEXT:   %vuge = fcmp uge <4 x float> %a1, %a2
; CHECK-NEXT:   %vult = fcmp ult <4 x float> %a1, %a2
; CHECK-NEXT:   %vule = fcmp ule <4 x float> %a1, %a2
; CHECK-NEXT:   %vune = fcmp une <4 x float> %a1, %a2
; CHECK-NEXT:   %vuno = fcmp uno <4 x float> %a1, %a2
; CHECK-NEXT:   %vtrue = fcmp true <4 x float> %a1, %a2
; CHECK-NEXT:   ret <4 x i1> %voeq
; CHECK-NEXT: }

; NOIR: Total across all functions
