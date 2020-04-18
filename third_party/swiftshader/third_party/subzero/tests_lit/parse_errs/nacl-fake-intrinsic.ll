; Tests that we don't get fooled by a fake NaCl intrinsic.

; REQUIRES: allow_dump

; RUN: %p2i --expect-fail -i %s --insts --args \
; RUN:      -verbose=inst -allow-externally-defined-symbols \
; RUN:   | FileCheck %s

declare i32 @llvm.fake.i32(i32)

; CHECK: Error({{.*}}): Invalid intrinsic name: llvm.fake.i32
