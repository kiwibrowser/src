; Tests malformed insertelement and extractelement vector instructions.

; RUN: %if --need=allow_dump --command \
; RUN:   %p2i --expect-fail -i %s --allow-pnacl-reader-error-recovery \
; RUN:   --filetype=obj -o /dev/null --args -notranslate \
; RUN:   | %if --need=allow_dump --command FileCheck %s

; RUN: %if --need=no_dump --command \
; RUN:   %p2i --expect-fail -i %s --allow-pnacl-reader-error-recovery \
; RUN:   --filetype=obj -o /dev/null --args -notranslate \
; RUN:   | %if --need=no_dump --command FileCheck %s --check-prefix=MIN

define void @ExtractV4xi1(<4 x i1> %v, i32 %i) {
  %e0 = extractelement <4 x i1> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 4 3>
  %e1 = extractelement <4 x i1> %v, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  %e2 = extractelement <4 x i1> %v, i32 9
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record:  <6 6 3>
  ret void
}

define void @ExtractV8xi1(<8 x i1> %v, i32 %i) {
  %e0 = extractelement <8 x i1> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 4 3>
  %e1 = extractelement <8 x i1> %v, i32 8;
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  %e2 = extractelement <8 x i1> %v, i32 9;
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 6 3>
  ret void
}

define void @ExtractV16xi1(<16 x i1> %v, i32 %i) {
  %e0 = extractelement <16 x i1> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 4 3>
  %e1 = extractelement <16 x i1> %v, i32 16
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  %e2 = extractelement <16 x i1> %v, i32 24
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 6 3>
  ret void
}

define void @ExtractV16xi8(<16 x i8> %v, i32 %i) {
  %e0 = extractelement <16 x i8> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 4 3>
  %e1 = extractelement <16 x i8> %v, i32 16
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  %e2 = extractelement <16 x i8> %v, i32 71
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 6 3>
  ret void
}

define void @ExtractV8xi16(<8 x i16> %v, i32 %i) {
  %e0 = extractelement <8 x i16> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 4 3>
  %e1 = extractelement <8 x i16> %v, i32 8
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  %e2 = extractelement <8 x i16> %v, i32 15
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 6 3>
  ret void
}

define i32 @ExtractV4xi32(<4 x i32> %v, i32 %i) {
  %e0 = extractelement <4 x i32> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 4 3>
  %e1 = extractelement <4 x i32> %v, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  %e2 = extractelement <4 x i32> %v, i32 17
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 6 3>
  ret i32 %e0
}

define float @ExtractV4xfloat(<4 x float> %v, i32 %i) {
  %e0 = extractelement <4 x float> %v, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <6 3 2>
  %e1 = extractelement <4 x float> %v, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 4 2>
  %e2 = extractelement <4 x float> %v, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <6 5 3>
  ret float %e2
}

define <4 x i1> @InsertV4xi1(<4 x i1> %v, i32 %i) {
  %r0 = insertelement <4 x i1> %v, i1 1, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 1 4>
  %r1 = insertelement <4 x i1> %v, i1 1, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 2 4>
  %r2 = insertelement <4 x i1> %v, i1 1, i32 7
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 3 4>
  ret <4 x i1> %r2
}

define <8 x i1> @InsertV8xi1(<8 x i1> %v, i32 %i) {
  %r0 = insertelement <8 x i1> %v, i1 0, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 1 4>
  %r1 = insertelement <8 x i1> %v, i1 0, i32 8
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 2 4>
  %r2 = insertelement <8 x i1> %v, i1 0, i32 88
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 3 4>
  ret <8 x i1> %r2
}

define <16 x i1> @InsertV16xi1(<16 x i1> %v, i32 %i) {
  %r = insertelement <16 x i1> %v, i1 1, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 1 4>
  ret <16 x i1> %r
  %r1 = insertelement <16 x i1> %v, i1 1, i32 16
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 2 4>
  %r2 = insertelement <16 x i1> %v, i1 1, i32 31
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 3 4>
  ret <16 x i1> %r2
}

define <16 x i8> @InsertV16xi8(<16 x i8> %v, i32 %i) {
  %r0 = insertelement <16 x i8> %v, i8 34, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 1 4>
  %r1 = insertelement <16 x i8> %v, i8 34, i32 16
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 2 4>
  %r2 = insertelement <16 x i8> %v, i8 34, i32 19
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 3 4>
  ret <16 x i8> %r0
}

define <8 x i16> @InsertV8xi16(<8 x i16> %v, i32 %i) {
  %r0 = insertelement <8 x i16> %v, i16 289, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 1 4>
  %r1 = insertelement <8 x i16> %v, i16 289, i32 8
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 2 4>
  %r2 = insertelement <8 x i16> %v, i16 289, i32 19
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 3 4>
  ret <8 x i16> %r1
}

define <4 x i32> @InsertV4xi32(<4 x i32> %v, i32 %i) {
  %r0 = insertelement <4 x i32> %v, i32 54545454, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 3 4>
  %r1 = insertelement <4 x i32> %v, i32 54545454, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 4 3>
  %r2 = insertelement <4 x i32> %v, i32 54545454, i32 9
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 5 3>
  ret <4 x i32> %r2
}

define <4 x float> @InsertV4xfloat(<4 x float> %v, i32 %i) {
  %r0 = insertelement <4 x float> %v, float 3.0, i32 %i
; CHECK: Error{{.*}} not {{.*}} constant
; MIN: Error{{.*}} Invalid function record: <7 5 1 4>
  %r1 = insertelement <4 x float> %v, float 3.0, i32 4
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 6 2 4>
  %r2 = insertelement <4 x float> %v, float 3.0, i32 44
; CHECK: Error{{.*}} not in range
; MIN: Error{{.*}} Invalid function record: <7 7 3 4>
  ret <4 x float> %r2
}
