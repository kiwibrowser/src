; This tests that unreachable basic blocks are pruned from the CFG, so that
; liveness analysis doesn't detect inconsistencies.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:      -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:      -allow-externally-defined-symbols | FileCheck %s

declare void @abort()

define internal i32 @unreachable_block() {
entry:
  ; ret_val has no reaching uses and so its assignment may be
  ; dead-code eliminated.
  %ret_val = add i32 undef, undef
  call void @abort()
  unreachable
label:
  ; ret_val has no reaching definitions, causing an inconsistency in
  ; liveness analysis.
  ret i32 %ret_val
}

; CHECK-LABEL: unreachable_block
