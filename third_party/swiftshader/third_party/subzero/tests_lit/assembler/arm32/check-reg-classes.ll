; Show that all register classes are defined for ARM32.


; REQUIRES: allow_dump
; RUN: %p2i --filetype=asm -i %s --target=arm32 --args -Om1 \
; RUN:   --verbose=registers  | FileCheck %s

define internal void @f() {
  ret void
}

; CHECK: Registers available for register allocation:
