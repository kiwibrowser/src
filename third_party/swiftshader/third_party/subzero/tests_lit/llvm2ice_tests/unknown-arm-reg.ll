; Show that we complain if an unknown register is specified on the command line.

; Compile using standalone assembler.
; RUN: %p2i --expect-fail --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   -reg-use r2,xx9x,r5,yy28 -reg-exclude s1,sq5 2>&1 \
; RUN:   | FileCheck %s

define void @foo() {
  ret void
}

; CHECK: LLVM ERROR: Unrecognized use/exclude registers: xx9x yy28 sq5
