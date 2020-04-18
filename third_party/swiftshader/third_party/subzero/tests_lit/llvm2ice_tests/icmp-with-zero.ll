; Simple test of non-fused compare/branch.

; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i --filetype=obj --disassemble -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck --check-prefix=OPTM1 %s

define internal void @icmpEqZero64() {
entry:
  %cmp = icmp eq i64 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpEqZero64
; CHECK: or
; CHECK-NOT: set
; OPTM1-LABEL: icmpEqZero64
; OPTM1: or
; OPTM1-NEXT: sete

define internal void @icmpNeZero64() {
entry:
  %cmp = icmp ne i64 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpNeZero64
; CHECK: or
; CHECK-NOT: set
; OPTM1-LABEL: icmpNeZero64
; OPTM1: or
; OPTM1-NEXT: setne

define internal void @icmpSgeZero64() {
entry:
  %cmp = icmp sge i64 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpSgeZero64
; CHECK: test eax,0x80000000
; CHECK-NOT: sete
; OPTM1-LABEL: icmpSgeZero64
; OPTM1: test eax,0x80000000
; OPTM1-NEXT: sete

define internal void @icmpSltZero64() {
entry:
  %cmp = icmp slt i64 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpSltZero64
; CHECK: test eax,0x80000000
; CHECK-NOT: setne
; OPTM1-LABEL: icmpSltZero64
; OPTM1: test eax,0x80000000
; OPTM1-NEXT: setne

define internal void @icmpUltZero64() {
entry:
  %cmp = icmp ult i64 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUltZero64
; CHECK: mov [[RESULT:.*]],0x0
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUltZero64
; OPTM1: mov [[RESULT:.*]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0

define internal void @icmpUgeZero64() {
entry:
  %cmp = icmp uge i64 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUgeZero64
; CHECK: mov [[RESULT:.*]],0x1
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUgeZero64
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp [[RESULT]],0x0

define internal void @icmpUltZero32() {
entry:
  %cmp = icmp ult i32 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cmp_ext = zext i1 %cmp to i32
  call void @use(i32 %cmp_ext)
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUltZero32
; CHECK: mov [[RESULT:.*]],0x0
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUltZero32
; OPTM1: mov [[RESULT:.*]],0x0
; OPTM1: cmp [[RESULT]],0x0

define internal void @icmpUgeZero32() {
entry:
  %cmp = icmp uge i32 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cmp_ext = zext i1 %cmp to i32
  call void @use(i32 %cmp_ext)
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUgeZero32
; CHECK: mov [[RESULT:.*]],0x1
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUgeZero32
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp [[RESULT]],0x0

define internal void @icmpUltZero16() {
entry:
  %cmp = icmp ult i16 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cmp_ext = zext i1 %cmp to i32
  call void @use(i32 %cmp_ext)
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUltZero16
; CHECK: mov [[RESULT:.*]],0x0
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUltZero16
; OPTM1: mov [[RESULT:.*]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0

define internal void @icmpUgeZero16() {
entry:
  %cmp = icmp uge i16 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cmp_ext = zext i1 %cmp to i32
  call void @use(i32 %cmp_ext)
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUgeZero16
; CHECK: mov [[RESULT:.*]],0x1
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUgeZero16
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp [[RESULT]],0x0

define internal void @icmpUltZero8() {
entry:
  %cmp = icmp ult i8 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cmp_ext = zext i1 %cmp to i32
  call void @use(i32 %cmp_ext)
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUltZero8
; CHECK: mov [[RESULT:.*]],0x0
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUltZero8
; OPTM1: mov [[RESULT:.*]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0

define internal void @icmpUgeZero8() {
entry:
  %cmp = icmp uge i8 123, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %cmp_ext = zext i1 %cmp to i32
  call void @use(i32 %cmp_ext)
  br label %if.end

if.end:                                          ; preds = %if.then, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpUgeZero8
; CHECK: mov [[RESULT:.*]],0x1
; CHECK-NEXT: cmp [[RESULT]],0x0
; OPTM1-LABEL: icmpUgeZero8
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp [[RESULT]],0x0

declare void @func()
declare void @use(i32)
