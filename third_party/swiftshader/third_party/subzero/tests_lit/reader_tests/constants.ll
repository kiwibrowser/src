; Test handling of constants in function blocks.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @TestIntegers() {
entry:
; CHECK: entry:

  ; Test various sized integers
  %v0 = or i1 true, false
; CHECK-NEXT:   %v0 = or i1 true, false

  %v1 = add i8 0, 0
; CHECK-NEXT:   %v1 = add i8 0, 0

  %v2 = add i8 5, 0
; CHECK-NEXT:   %v2 = add i8 5, 0

  %v3 = add i8 -5, 0
; CHECK-NEXT:   %v3 = add i8 -5, 0

  %v4 = and i16 10, 0
; CHECK-NEXT:   %v4 = and i16 10, 0

  %v5 = add i16 -10, 0
; CHECK-NEXT:   %v5 = add i16 -10, 0

  %v6 = add i32 20, 0
; CHECK-NEXT:   %v6 = add i32 20, 0

  %v7 = add i32 -20, 0
; CHECK-NEXT:   %v7 = add i32 -20, 0

  %v8 = add i64 30, 0
; CHECK-NEXT:   %v8 = add i64 30, 0

  %v9 = add i64 -30, 0
; CHECK-NEXT:   %v9 = add i64 -30, 0

  ; Test undefined integer values.
  %v10 = xor i1 undef, false
; CHECK-NEXT:   %v10 = xor i1 undef, false

  %v11 = add i8 undef, 0
; CHECK-NEXT:   %v11 = add i8 undef, 0

  %v12 = add i16 undef, 0
; CHECK-NEXT:   %v12 = add i16 undef, 0

  %v13 = add i32 undef, 0
; CHECK-NEXT:   %v13 = add i32 undef, 0

  %v14 = add i64 undef, 0
; CHECK-NEXT:   %v14 = add i64 undef, 0

  ret void
; CHECK-NEXT:   ret void

}

define internal void @TestFloats() {
entry:
; CHECK: entry:

  ; Test float and double constants
  %v0 = fadd float 1.0, 0.0
; CHECK-NEXT:   %v0 = fadd float 1.000000e+00, 0.000000e+00

  %v1 = fadd double 1.0, 0.0
; CHECK-NEXT:   %v1 = fadd double 1.000000e+00, 0.000000e+00

  %v2 = fsub float 7.000000e+00, 8.000000e+00
; CHECK-NEXT:   %v2 = fsub float 7.000000e+00, 8.000000e+00

  %v3 = fsub double 5.000000e+00, 6.000000e+00
; CHECK-NEXT:   %v3 = fsub double 5.000000e+00, 6.000000e+00

  ; Test undefined float and double.
  %v4 = fadd float undef, 0.0
; CHECK-NEXT:   %v4 = fadd float undef, 0.000000e+00

  %v5 = fsub double undef, 6.000000e+00
; CHECK-NEXT:   %v5 = fsub double undef, 6.000000e+00

  ; Test special floating point constants. Note: LLVM assembly appears
  ; to use 64-bit integer constants for both float and double.

  ; Generated from NAN in <math.h>
  %v6 = fadd float 0x7FF8000000000000, 0.0
; CHECK-NEXT:   %v6 = fadd float nan, 0.000000e+00

  ; Generated from -NAN in <math.h>
  %v7 = fadd float 0xFFF8000000000000, 0.0
; CHECK-NEXT:   %v7 = fadd float -nan, 0.000000e+00

  ; Generated from INFINITY in <math.h>
  %v8 = fadd float 0x7FF0000000000000, 0.0
; CHECK-NEXT:   %v8 = fadd float inf, 0.000000e+00

  ; Generated from -INFINITY in <math.h>
  %v9 = fadd float 0xFFF0000000000000, 0.0
; CHECK-NEXT:   %v9 = fadd float -inf, 0.000000e+00

  ; Generated from FLT_MIN in <float.h>
  %v10 = fadd float 0x381000000000000000, 0.0
; CHECK-NEXT:   %v10 = fadd float 0.000000e+00, 0.000000e+00

  ; Generated from -FLT_MIN in <float.h>
  %v11 = fadd float 0xb81000000000000000, 0.0
; CHECK-NEXT:   %v11 = fadd float 0.000000e+00, 0.000000e+00

  ; Generated from FLT_MAX in <float.h>
  %v12 = fadd float 340282346638528859811704183484516925440.000000, 0.0
; CHECK-NEXT:   %v12 = fadd float 3.402823e+38, 0.000000e+00

  ; Generated from -FLT_MAX in <float.h>
  %v13 = fadd float -340282346638528859811704183484516925440.000000, 0.0
; CHECK-NEXT:   %v13 = fadd float -3.402823e+38, 0.000000e+00

  ; Generated from NAN in <math.h>
  %v14 = fadd double 0x7FF8000000000000, 0.0
; CHECK-NEXT:   %v14 = fadd double nan, 0.000000e+00

  ; Generated from -NAN in <math.h>
  %v15 = fadd double 0xFFF8000000000000, 0.0
; CHECK-NEXT:   %v15 = fadd double -nan, 0.000000e+00

  ; Generated from INFINITY in <math.h>
  %v16 = fadd double 0x7FF0000000000000, 0.0
; CHECK-NEXT:   %v16 = fadd double inf, 0.000000e+00

  ; Generated from -INFINITY in <math.h>
  %v17 = fadd double 0xFFF0000000000000, 0.0
; CHECK-NEXT:   %v17 = fadd double -inf, 0.000000e+00

  ; Generated from DBL_MIN in <float.h>
  %v18 = fadd double 0x0010000000000000, 0.0
; CHECK-NEXT:   %v18 = fadd double 2.225074e-308, 0.000000e+00

  ; Generated from -DBL_MIN in <float.h>
  %v19 = fadd double 0x8010000000000000, 0.0
; CHECK-NEXT:   %v19 = fadd double -2.225074e-308, 0.000000e+00

  ; Generated from DBL_MAX in <float.h>
  %v20 = fadd double 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000, 0.0
; CHECK-NEXT:   %v20 = fadd double 1.797693e+308, 0.000000e+00

  ; Generated from -DBL_MAX in <float.h>
  %v21 = fadd double -179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000, 0.0
; CHECK-NEXT:   %v21 = fadd double -1.797693e+308, 0.000000e+00

  ret void
; CHECK-NEXT:   ret void
}

; NOIR: Total across all functions
