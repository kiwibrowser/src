; Tests that we name unnamed global addresses.

; Check that Subzero's bitcode reader handles renaming correctly.
; RUN: %p2i --no-local-syms -i %s --insts | FileCheck %s
; RUN: %l2i --no-local-syms -i %s --insts | %ifl FileCheck %s

; RUN: %l2i --no-local-syms -i %s --insts --args --exit-success \
; RUN:      -default-function-prefix=h -default-global-prefix=g \
; RUN:      | %ifl FileCheck --check-prefix=BAD %s

; RUN: %p2i --no-local-syms -i %s --insts --args --exit-success \
; RUN:      -default-function-prefix=h -default-global-prefix=g \
; RUN:      | FileCheck --check-prefix=BAD %s

; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

; TODO(kschimpf) Check global variable declarations, once generated.

@0 = internal global [4 x i8] zeroinitializer, align 4
@1 = internal constant [10 x i8] c"Some stuff", align 1
@g = internal global [4 x i8] zeroinitializer, align 4

define internal i32 @2(i32 %v) {
  ret i32 %v
}

; CHECK:      define internal i32 @Function(i32 %__0) {
; CHECK-NEXT: __0:
; CHECK-NEXT:   ret i32 %__0
; CHECK-NEXT: }

define internal void @hg() {
  ret void
}


; CHECK-NEXT: define internal void @hg() {
; CHECK-NEXT: __0:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @3() {
  ret void
}

; CHECK-NEXT: define internal void @Function1() {
; CHECK-NEXT: __0:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @h5() {
  ret void
}

; CHECK-NEXT: define internal void @h5() {
; CHECK-NEXT: __0:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; BAD: Warning : Default global prefix 'g' potentially conflicts with name 'g'.
; BAD: Warning : Default function prefix 'h' potentially conflicts with name 'h5'.

; NOIR: Total across all functions
