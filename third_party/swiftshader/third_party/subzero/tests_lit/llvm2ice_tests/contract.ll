; This tests that an empty node pointing to itself is not contracted.
; https://code.google.com/p/nativeclient/issues/detail?id=4307
;
; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   | FileCheck %s

define internal void @SimpleBranch() {
label0:
  br label %label2
label1:
  br label %label1
label2:
  br label %label1
}

; CHECK-LABEL: SimpleBranch
; CHECK-NEXT: jmp 0 <SimpleBranch>
