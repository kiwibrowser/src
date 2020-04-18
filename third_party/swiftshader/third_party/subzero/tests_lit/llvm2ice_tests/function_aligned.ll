; Test that functions are aligned to the NaCl bundle alignment.
; We could be smarter and only do this for indirect call targets
; but typically you want to align functions anyway.
; Also, we are currently using hlts for non-executable padding.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 | FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal void @foo() {
  ret void
}
; CHECK-LABEL: foo
; CHECK-NEXT: 0: {{.*}} ret
; CHECK-NEXT: 1: {{.*}} hlt
; ARM32-LABEL: foo
; ARM32-NEXT: 0: {{.*}} bx lr
; ARM32-NEXT: 4: e7fedef0 udf
; ARM32-NEXT: 8: e7fedef0 udf
; ARM32-NEXT: c: e7fedef0 udf
; MIPS32-LABEL: foo
; MIPS32: 0: {{.*}} jr ra
; MIPS32-NEXT: 4: {{.*}} nop

define internal void @bar() {
  ret void
}
; CHECK-LABEL: bar
; CHECK-NEXT: 20: {{.*}} ret
; ARM32-LABEL: bar
; ARM32-NEXT: 10: {{.*}} bx lr
; MIPS32-LABEL: bar
; MIPS32: 10: {{.*}} jr ra
; MIPS32-NEXT: 14: {{.*}} nop
