; Test switch instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @testDefaultSwitch(i32 %a) {
entry:
  switch i32 %a, label %exit [
  ]
exit:
  ret void
}

; CHECK:      define internal void @testDefaultSwitch(i32 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i32 %a, label %exit [
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i32 @testSwitch(i32 %a) {
entry:
  switch i32 %a, label %sw.default [
    i32 1, label %sw.epilog
    i32 2, label %sw.epilog
    i32 3, label %sw.epilog
    i32 7, label %sw.bb1
    i32 8, label %sw.bb1
    i32 15, label %sw.bb2
    i32 14, label %sw.bb2
  ]

sw.default:                                       ; preds = %entry
  %add = add i32 %a, 27
  br label %sw.epilog

sw.bb1:                                           ; preds = %entry, %entry
  %phitmp = sub i32 21, %a
  br label %sw.bb2

sw.bb2:                                           ; preds = %sw.bb1, %entry, %entry
  %result.0 = phi i32 [ 1, %entry ], [ 1, %entry ], [ %phitmp, %sw.bb1 ]
  br label %sw.epilog

sw.epilog:                                        ; preds = %sw.bb2, %sw.default, %entry, %entry, %entry
  %result.1 = phi i32 [ %add, %sw.default ], [ %result.0, %sw.bb2 ], [ 17, %entry ], [ 17, %entry ], [ 17, %entry ]
  ret i32 %result.1
}

; CHECK-NEXT:      define internal i32 @testSwitch(i32 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i32 %a, label %sw.default [
; CHECK-NEXT:     i32 1, label %sw.epilog
; CHECK-NEXT:     i32 2, label %sw.epilog
; CHECK-NEXT:     i32 3, label %sw.epilog
; CHECK-NEXT:     i32 7, label %sw.bb1
; CHECK-NEXT:     i32 8, label %sw.bb1
; CHECK-NEXT:     i32 15, label %sw.bb2
; CHECK-NEXT:     i32 14, label %sw.bb2
; CHECK-NEXT:   ]
; CHECK-NEXT: sw.default:
; CHECK-NEXT:   %add = add i32 %a, 27
; CHECK-NEXT:   br label %sw.epilog
; CHECK-NEXT: sw.bb1:
; CHECK-NEXT:   %phitmp = sub i32 21, %a
; CHECK-NEXT:   br label %sw.bb2
; CHECK-NEXT: sw.bb2:
; CHECK-NEXT:   %result.0 = phi i32 [ 1, %entry ], [ 1, %entry ], [ %phitmp, %sw.bb1 ]
; CHECK-NEXT:   br label %sw.epilog
; CHECK-NEXT: sw.epilog:
; CHECK-NEXT:   %result.1 = phi i32 [ %add, %sw.default ], [ %result.0, %sw.bb2 ], [ 17, %entry ], [ 17, %entry ], [ 17, %entry ]
; CHECK-NEXT:   ret i32 %result.1
; CHECK-NEXT: }

define internal void @testSignedI32Values(i32 %a) {
entry:
  switch i32 %a, label %labelDefault [
  i32 0, label %label0
  i32 -1, label %labelM1
  i32 3, label %labelOther
  i32 -3, label %labelOther
  i32 -2147483648, label %labelMin  ; min signed i32
  i32 2147483647, label %labelMax   ; max signed i32
  ]
labelDefault:
  ret void
label0:
  ret void
labelM1:
  ret void
labelMin:
  ret void
labelMax:
  ret void
labelOther:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI32Values(i32 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i32 %a, label %labelDefault [
; CHECK-NEXT:     i32 0, label %label0
; CHECK-NEXT:     i32 -1, label %labelM1
; CHECK-NEXT:     i32 3, label %labelOther
; CHECK-NEXT:     i32 -3, label %labelOther
; CHECK-NEXT:     i32 -2147483648, label %labelMin
; CHECK-NEXT:     i32 2147483647, label %labelMax
; CHECK-NEXT:   ]
; CHECK-NEXT: labelDefault:
; CHECK-NEXT:   ret void
; CHECK-NEXT: label0:
; CHECK-NEXT:   ret void
; CHECK-NEXT: labelM1:
; CHECK-NEXT:   ret void
; CHECK-NEXT: labelMin:
; CHECK-NEXT:   ret void
; CHECK-NEXT: labelMax:
; CHECK-NEXT:   ret void
; CHECK-NEXT: labelOther:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross signed i32 size boundaries.
define internal void @testSignedI32Boundary(i32 %a) {
entry:
  switch i32 %a, label %exit [
  i32 -2147483649, label %exit  ; min signed i32 - 1
  i32 2147483648, label %exit   ; max signed i32 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI32Boundary(i32 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i32 %a, label %exit [
; CHECK-NEXT:     i32 2147483647, label %exit
; CHECK-NEXT:     i32 -2147483648, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testUnsignedI32Values(i32 %a) {
entry:
  switch i32 %a, label %exit [
  i32 0, label %exit
  i32 2147483647, label %exit   ; max signed i32
  i32 4294967295, label %exit   ; max unsigned i32
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI32Values(i32 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i32 %a, label %exit [
; CHECK-NEXT:     i32 0, label %exit
; CHECK-NEXT:     i32 2147483647, label %exit
;                 ; Note that -1 is signed version of 4294967295
; CHECK-NEXT:     i32 -1, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross unsigned i32 boundaries.
define internal void @testUnsignedI32Boundary(i32 %a) {
entry:
  switch i32 %a, label %exit [
  i32 4294967296, label %exit   ; max unsigned i32 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI32Boundary(i32 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i32 %a, label %exit [
; CHECK-NEXT:     i32 0, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testSignedI64Values(i64 %a) {
entry:
  switch i64 %a, label %exit [
  i64 0, label %exit
  i64 -9223372036854775808, label %exit   ; min signed i64
  i64 9223372036854775807, label %exit    ; max signed i64
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI64Values(i64 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i64 %a, label %exit [
; CHECK-NEXT:     i64 0, label %exit
; CHECK-NEXT:     i64 -9223372036854775808, label %exit
; CHECK-NEXT:     i64 9223372036854775807, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross signed i64 size boundaries.
define internal void @testSignedI64Boundary(i64 %a) {
entry:
  switch i64 %a, label %exit [
  i64 0, label %exit
  i64 -9223372036854775809, label %exit   ; min signed i64 - 1
  i64 9223372036854775808, label %exit   ; max signed i64 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI64Boundary(i64 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i64 %a, label %exit [
; CHECK-NEXT:     i64 0, label %exit
; CHECK-NEXT:     i64 9223372036854775807, label %exit
; CHECK-NEXT:     i64 -9223372036854775808, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testUnsignedI64Values(i64 %a) {
entry:
  switch i64 %a, label %exit [
  i64 0, label %exit
  i64 9223372036854775807, label %exit   ; max signed i64
  i64 18446744073709551615, label %exit   ; max unsigned i64
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI64Values(i64 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i64 %a, label %exit [
; CHECK-NEXT:     i64 0, label %exit
; CHECK-NEXT:     i64 9223372036854775807, label %exit
; CHECK-NEXT:     i64 -1, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross unsigned i64 size boundaries.
define internal void @testUnsignedI64Boundary(i64 %a) {
entry:
  switch i64 %a, label %exit [
  i64 18446744073709551616, label %exit   ; max unsigned i64 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI64Boundary(i64 %a) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   switch i64 %a, label %exit [
; CHECK-NEXT:     i64 0, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testSignedI16Values(i32 %p) {
entry:
  %a = trunc i32 %p to i16
  switch i16 %a, label %exit [
  i16 0, label %exit
  i16 -1, label %exit
  i16 3, label %exit
  i16 -3, label %exit
  i16 -32768, label %exit   ; min signed i16
  i16 32767, label %exit   ; max unsigned i16
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI16Values(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i16
; CHECK-NEXT:   switch i16 %a, label %exit [
; CHECK-NEXT:     i16 0, label %exit
; CHECK-NEXT:     i16 -1, label %exit
; CHECK-NEXT:     i16 3, label %exit
; CHECK-NEXT:     i16 -3, label %exit
; CHECK-NEXT:     i16 -32768, label %exit
; CHECK-NEXT:     i16 32767, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross signed i16 size boundaries.
define internal void @testSignedI16Boundary(i32 %p) {
entry:
  %a = trunc i32 %p to i16
  switch i16 %a, label %exit [
  i16 -32769, label %exit   ; min signed i16 - 1
  i16 32768, label %exit   ; max unsigned i16 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI16Boundary(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i16
; CHECK-NEXT:   switch i16 %a, label %exit [
; CHECK-NEXT:     i16 32767, label %exit
; CHECK-NEXT:     i16 -32768, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testUnsignedI16Values(i32 %p) {
entry:
  %a = trunc i32 %p to i16
  switch i16 %a, label %exit [
  i16 0, label %exit
  i16 32767, label %exit   ; max signed i16
  i16 65535, label %exit   ; max unsigned i16
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI16Values(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i16
; CHECK-NEXT:   switch i16 %a, label %exit [
; CHECK-NEXT:     i16 0, label %exit
; CHECK-NEXT:     i16 32767, label %exit
;                 ; Note that -1 is signed version of 65535
; CHECK-NEXT:     i16 -1, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross unsigned i16 size boundaries.
define internal void @testUnsignedI16Boundary(i32 %p) {
entry:
  %a = trunc i32 %p to i16
  switch i16 %a, label %exit [
  i16 65536, label %exit   ; max unsigned i16 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI16Boundary(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i16
; CHECK-NEXT:   switch i16 %a, label %exit [
; CHECK-NEXT:     i16 0, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testSignedI8Values(i32 %p) {
entry:
  %a = trunc i32 %p to i8
  switch i8 %a, label %exit [
  i8 0, label %exit
  i8 -1, label %exit
  i8 3, label %exit
  i8 -3, label %exit
  i8 -128, label %exit   ; min signed i8
  i8 127, label %exit   ; max unsigned i8
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI8Values(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i8
; CHECK-NEXT:   switch i8 %a, label %exit [
; CHECK-NEXT:     i8 0, label %exit
; CHECK-NEXT:     i8 -1, label %exit
; CHECK-NEXT:     i8 3, label %exit
; CHECK-NEXT:     i8 -3, label %exit
; CHECK-NEXT:     i8 -128, label %exit
; CHECK-NEXT:     i8 127, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross signed i8 size boundaries.
define internal void @testSignedI8Boundary(i32 %p) {
entry:
  %a = trunc i32 %p to i8
  switch i8 %a, label %exit [
  i8 -129, label %exit   ; min signed i8 - 1
  i8 128, label %exit   ; max unsigned i8 + 1
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testSignedI8Boundary(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i8
; CHECK-NEXT:   switch i8 %a, label %exit [
; CHECK-NEXT:     i8 127, label %exit
; CHECK-NEXT:     i8 -128, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }


define internal void @testUnsignedI8Values(i32 %p) {
entry:
  %a = trunc i32 %p to i8
  switch i8 %a, label %exit [
  i8 0, label %exit
  i8 127, label %exit   ; max signed i8
  i8 255, label %exit   ; max unsigned i8
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI8Values(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i8
; CHECK-NEXT:   switch i8 %a, label %exit [
; CHECK-NEXT:     i8 0, label %exit
; CHECK-NEXT:     i8 127, label %exit
;                 ; Note that -1 is signed version of 255
; CHECK-NEXT:     i8 -1, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Test values that cross unsigned i8 size boundaries.
define internal void @testUnsignedI8Boundary(i32 %p) {
entry:
  %a = trunc i32 %p to i8
  switch i8 %a, label %exit [
  i8 256, label %exit   ; max unsigned i8
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testUnsignedI8Boundary(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i8
; CHECK-NEXT:   switch i8 %a, label %exit [
; CHECK-NEXT:     i8 0, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @testI1Values(i32 %p) {
entry:
  %a = trunc i32 %p to i1
  switch i1 %a, label %exit [
  i1 true, label %exit
  i1 false, label %exit
  ]
exit:
  ret void
}

; CHECK-NEXT: define internal void @testI1Values(i32 %p) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = trunc i32 %p to i1
; CHECK-NEXT:   switch i1 %a, label %exit [
; CHECK-NEXT:     i1 -1, label %exit
; CHECK-NEXT:     i1 0, label %exit
; CHECK-NEXT:   ]
; CHECK-NEXT: exit:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; NOIR: Total across all functions
