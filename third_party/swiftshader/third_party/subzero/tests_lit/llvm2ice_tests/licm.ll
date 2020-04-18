; Tests if the licm flag successfully hoists the add from loop0 to entry

; RUN: %p2i -i %s --filetype=obj --disassemble --target x8664 --args \
; RUN: -O2 -licm | FileCheck --check-prefix ENABLE %s

; RUN: %p2i -i %s --filetype=obj --disassemble --target x8664 --args \
; RUN: -O2 | FileCheck --check-prefix NOENABLE %s

define internal void @dummy() {
entry:
  ret void
}
define internal i32 @test_licm(i32 %a32, i32 %b, i32 %c) {
entry:
  %a = trunc i32 %a32 to i1
  br label %loop0
loop0:                               ; <-+
  call void @dummy()                 ;   |
  %add1 = add i32 %b, %c             ;   |
  br label %loop1                    ;   |
loop1:                               ;   |
  br i1 %a, label %loop0, label %out ; --+
out:
  ret i32 %add1
}

; CHECK-LABEL: test_licm

; ENABLE: add
; ENABLE: call

; NOENABLE: call
; NOENABLE-NEXT: mov
; NOENABLE-NEXT: add

; CHECK: ret