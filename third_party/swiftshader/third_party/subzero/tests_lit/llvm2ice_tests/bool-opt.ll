; Trivial smoke test of icmp without fused branch opportunity.

; RUN: %p2i -i %s --filetype=obj --disassemble --args \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

; Check that correct addressing modes are used for comparing two
; immediates.
define internal void @testIcmpImm() {
entry:
  %cmp = icmp eq i32 1, 2
  %cmp_ext = zext i1 %cmp to i32
  tail call void @use(i32 %cmp_ext)
  ret void
}
; CHECK-LABEL: testIcmpImm
; CHECK-NOT: cmp {{[0-9]+}},

declare void @use(i32)
