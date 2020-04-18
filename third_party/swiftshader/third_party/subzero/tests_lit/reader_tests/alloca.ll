; Test if we can read alloca instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

; Show examples where size is defined by a constant.

define internal i32 @AllocaA0Size1() {
entry:
  %array = alloca i8, i32 1
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 1
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaA0Size2() {
entry:
  %array = alloca i8, i32 2
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 2
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaA0Size3() {
entry:
  %array = alloca i8, i32 3
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 3
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaA0Size4() {
entry:
  %array = alloca i8, i32 4
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 4
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaA1Size4(i32 %n) {
entry:
  %array = alloca i8, i32 4, align 1
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 4, align 1
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaA2Size4(i32 %n) {
entry:
  %array = alloca i8, i32 4, align 2
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 4, align 2
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaA8Size4(i32 %n) {
entry:
  %array = alloca i8, i32 4, align 8
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 4, align 8
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @Alloca16Size4(i32 %n) {
entry:
  %array = alloca i8, i32 4, align 16
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 4, align 16
; CHECK-NEXT:   ret i32 %array
}

; Show examples where size is not known at compile time.

define internal i32 @AllocaVarsizeA0(i32 %n) {
entry:
  %array = alloca i8, i32 %n
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 %n
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaVarsizeA1(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 1
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 %n, align 1
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaVarsizeA2(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 2
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 %n, align 2
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaVarsizeA4(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 4
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 %n, align 4
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaVarsizeA8(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 8
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 %n, align 8
; CHECK-NEXT:   ret i32 %array
}

define internal i32 @AllocaVarsizeA16(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 16
  %addr = ptrtoint i8* %array to i32
  ret i32 %addr

; CHECK:      entry:
; CHECK-NEXT:   %array = alloca i8, i32 %n, align 16
; CHECK-NEXT:   ret i32 %array
}

; NOIR: Total across all functions
