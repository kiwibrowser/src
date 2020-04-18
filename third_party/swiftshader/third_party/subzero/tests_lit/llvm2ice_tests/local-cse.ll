; Tests local-cse on x8632 and x8664

; RUN: %p2i -i %s --filetype=obj --disassemble --target x8632 --args \
; RUN: -O2 | FileCheck --check-prefix=X8632 \
; RUN: --check-prefix=X8632EXP %s

; RUN: %p2i -i %s --filetype=obj --disassemble --target x8632 --args \
; RUN: -O2 -lcse=0| FileCheck --check-prefix=X8632 --check-prefix X8632NOEXP %s

; RUN: %p2i -i %s --filetype=obj --disassemble --target x8664 --args \
; RUN: -O2 | FileCheck --check-prefix=X8664 \
; RUN: --check-prefix=X8664EXP %s

; RUN: %p2i -i %s --filetype=obj --disassemble --target x8664 --args \
; RUN: -O2 -lcse=0| FileCheck --check-prefix=X8664 --check-prefix X8664NOEXP %s


define internal i32 @local_cse_test(i32 %a, i32 %b) {
entry:
  %add1 = add i32 %b, %a
  %add2 = add i32 %b, %a
  %add3 = add i32 %add1, %add2
  ret i32 %add3
}

; X8632: add
; X8632: add
; X8632NOEXP: add
; X8632EXP-NOT: add
; X8632: ret

; X8664: add
; X8664: add
; X8664NOEXP: add
; X8664EXP-NOT: add
; X8664: ret