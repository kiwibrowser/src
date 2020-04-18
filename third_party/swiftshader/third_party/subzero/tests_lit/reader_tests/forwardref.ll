; Test use forward type references in function blocks.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN: llvm-as < %s | pnacl-freeze | pnacl-bcdis -no-records \
; RUN:              | FileCheck --check-prefix=DUMP %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal void @LoopCarriedDep() {
b0:
  %v0 = add i32 1, 2
  br label %b1
b1:
  %v1 = phi i32 [%v0, %b0], [%v2, %b1]
  %v2 = add i32 %v1, 1
  br label %b1
}

; CHECK:      define internal void @LoopCarriedDep() {
; CHECK-NEXT: b0:
; CHECK-NEXT:   %v0 = add i32 1, 2
; CHECK-NEXT:   br label %b1
; CHECK-NEXT: b1:
; CHECK-NEXT:   %v1 = phi i32 [ %v0, %b0 ], [ %v2, %b1 ]
; CHECK-NEXT:   %v2 = add i32 %v1, 1
; CHECK-NEXT:   br label %b1
; CHECK-NEXT: }

; Snippet of bitcode objdump with forward type reference (see "declare").
; DUMP:        function void @f0() {  // BlockID = 12
; DUMP-NEXT:     blocks 2;
; DUMP-NEXT:     constants {  // BlockID = 11
; DUMP-NEXT:       i32: <@a0>
; DUMP-NEXT:         %c0 = i32 1; <@a1>
; DUMP-NEXT:         %c1 = i32 2; <@a1>
; DUMP-NEXT:       }
; DUMP-NEXT:   %b0:
; DUMP-NEXT:     %v0 = add i32 %c0, %c1; <@a1>
; DUMP-NEXT:     br label %b1;
; DUMP-NEXT:   %b1:
; DUMP-NEXT:     declare i32 %v2; <@a6>
; DUMP-NEXT:     %v1 = phi i32 [%v0, %b0], [%v2, %b1];
; DUMP-NEXT:     %v2 = add i32 %v1, %c0; <@a1>
; DUMP-NEXT:     br label %b1;
; DUMP-NEXT:   }

define internal void @BackBranch(i32 %p0) {
b0:
  br label %b4
b1:
  %v0 = add i32 %p0, %v3
  br label %b6
b2:
  %v1 = add i32 %p0, %v4
  br label %b6
b3:
  %v2 = add i32 %p0, %v3 ; No forward declare, already done!
  br label %b6
b4:
  %v3 = add i32 %p0, %p0
  br i1 1, label %b1, label %b5
b5:
  %v4 = add i32 %v3, %p0
  br i1 1, label %b2, label %b3
b6:
  ret void
}

; CHECK:      define internal void @BackBranch(i32 %p0) {
; CHECK-NEXT: b0:
; CHECK-NEXT:   br label %b4
; CHECK-NEXT: b1:
; CHECK-NEXT:   %v0 = add i32 %p0, %v3
; CHECK-NEXT:   br label %b6
; CHECK-NEXT: b4:
; CHECK-NEXT:   %v3 = add i32 %p0, %p0
; CHECK-NEXT:   br label %b1
; CHECK-NEXT: b6:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; Snippet of bitcode objdump with forward type references (see "declare").
; DUMP:        function void @f1(i32 %p0) {  // BlockID = 12
; DUMP-NEXT:     blocks 7;
; DUMP-NEXT:     constants {  // BlockID = 11
; DUMP-NEXT:       i1: <@a0>
; DUMP-NEXT:         %c0 = i1 1; <@a1>
; DUMP-NEXT:       }
; DUMP-NEXT:   %b0:
; DUMP-NEXT:     br label %b4;
; DUMP-NEXT:   %b1:
; DUMP-NEXT:     declare i32 %v3; <@a6>
; DUMP-NEXT:     %v0 = add i32 %p0, %v3; <@a1>
; DUMP-NEXT:     br label %b6;
; DUMP-NEXT:   %b2:
; DUMP-NEXT:     declare i32 %v4; <@a6>
; DUMP-NEXT:     %v1 = add i32 %p0, %v4; <@a1>
; DUMP-NEXT:     br label %b6;
; DUMP-NEXT:   %b3:
; DUMP-NEXT:     %v2 = add i32 %p0, %v3; <@a1>
; DUMP-NEXT:     br label %b6;
; DUMP-NEXT:   %b4:
; DUMP-NEXT:     %v3 = add i32 %p0, %p0; <@a1>
; DUMP-NEXT:     br i1 %c0, label %b1, label %b5;
; DUMP-NEXT:   %b5:
; DUMP-NEXT:     %v4 = add i32 %v3, %p0; <@a1>
; DUMP-NEXT:     br i1 %c0, label %b2, label %b3;
; DUMP-NEXT:   %b6:
; DUMP-NEXT:     ret void; <@a3>
; DUMP-NEXT:   }

; NOIR: Total across all functions
