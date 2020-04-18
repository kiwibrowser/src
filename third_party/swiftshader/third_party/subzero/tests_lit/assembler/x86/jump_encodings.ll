; Tests various aspects of x86 branch encodings (near vs far,
; forward vs backward, using CFG labels, or local labels).

; Use -ffunction-sections so that the offsets reset for each function.
; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 \
; RUN:   -ffunction-sections | FileCheck %s

; Use atomic ops as filler, which shouldn't get optimized out.
declare void @llvm.nacl.atomic.store.i32(i32, i32*, i32)
declare i32 @llvm.nacl.atomic.load.i32(i32*, i32)
declare i32 @llvm.nacl.atomic.rmw.i32(i32, i32*, i32, i32)

define internal void @test_near_backward(i32 %iptr, i32 %val) {
entry:
  br label %next
next:
  %ptr = inttoptr i32 %iptr to i32*
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  br label %next2
next2:
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  %cmp = icmp ult i32 %val, 1
  br i1 %cmp, label %next2, label %next
}

; CHECK-LABEL: test_near_backward
; CHECK:      8: {{.*}}  mov DWORD PTR
; CHECK-NEXT: a: {{.*}}  mfence
; CHECK-NEXT: d: {{.*}}  mov DWORD PTR
; CHECK-NEXT: f: {{.*}}  mfence
; CHECK-NEXT: 12: {{.*}} cmp
; CHECK-NEXT: 15: 72 f6 jb d
; CHECK-NEXT: 17: eb ef jmp 8

; Test one of the backward branches being too large for 8 bits
; and one being just okay.
define internal void @test_far_backward1(i32 %iptr, i32 %val) {
entry:
  br label %next
next:
  %ptr = inttoptr i32 %iptr to i32*
  %tmp = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  br label %next2
next2:
  call void @llvm.nacl.atomic.store.i32(i32 %tmp, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  %cmp = icmp ugt i32 %val, 0
  br i1 %cmp, label %next2, label %next
}

; CHECK-LABEL: test_far_backward1
; CHECK:      8: {{.*}}  mov {{.*}},DWORD PTR [e{{[^s]}}
; CHECK-NEXT: a: {{.*}}  mov DWORD PTR
; CHECK-NEXT: c: {{.*}}  mfence
; CHECK: 85: 77 83 ja a
; CHECK-NEXT: 87: e9 7c ff ff ff jmp 8

; Same as test_far_backward1, but with the conditional branch being
; the one that is too far.
define internal void @test_far_backward2(i32 %iptr, i32 %val) {
entry:
  br label %next
next:
  %ptr = inttoptr i32 %iptr to i32*
  %tmp = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  %tmp2 = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  %tmp3 = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  %tmp4 = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  %tmp5 = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  br label %next2
next2:
  call void @llvm.nacl.atomic.store.i32(i32 %tmp, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %tmp2, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %tmp3, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %tmp4, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %tmp5, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  %cmp = icmp sle i32 %val, 0
  br i1 %cmp, label %next, label %next2
}

; CHECK-LABEL: test_far_backward2
; CHECK:      c:  {{.*}}  mov {{.*}},DWORD PTR [e{{[^s]}}
; CHECK:      14: {{.*}}  mov {{.*}},DWORD PTR
; CHECK-NEXT: 16: {{.*}}  mov DWORD PTR
; CHECK-NEXT: 18: {{.*}}  mfence
; CHECK: 8c: 0f 8e 7a ff ff ff jle c
; CHECK-NEXT: 92: eb 82 jmp 16

define internal void @test_near_forward(i32 %iptr, i32 %val) {
entry:
  br label %next1
next1:
  %ptr = inttoptr i32 %iptr to i32*
  %cmp = icmp ult i32 %val, 1
  br i1 %cmp, label %next3, label %next2
next2:
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  br label %next3
next3:
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  br label %next1
}
; Note: forward branches for non-local labels in Subzero currently use the fully
; relaxed form (4-byte offset) to avoid needing a relaxation pass.  When we use
; llvm-mc, it performs the relaxation pass and uses a 1-byte offset.
; CHECK-LABEL: test_near_forward
; CHECK:      [[BACKLABEL:[0-9a-f]+]]: {{.*}} cmp
; CHECK-NEXT: {{.*}} jb [[FORWARDLABEL:[0-9a-f]+]]
; CHECK-NEXT: {{.*}} mov DWORD PTR
; CHECK-NEXT: {{.*}} mfence
; CHECK-NEXT: [[FORWARDLABEL]]: {{.*}} mov DWORD PTR
; CHECK:      {{.*}} jmp [[BACKLABEL]]


; Unlike forward branches to cfg nodes, "local" forward branches
; always use a 1 byte displacement.
; Check local forward branches, followed by a near backward branch
; to make sure that the instruction size accounting for the forward
; branches are correct, by the time the backward branch is hit.
; A 64-bit compare happens to use local forward branches.
define internal void @test_local_forward_then_back(i64 %val64, i32 %iptr,
                                                   i32 %val) {
entry:
  br label %next
next:
  %ptr = inttoptr i32 %iptr to i32*
  call void @llvm.nacl.atomic.store.i32(i32 %val, i32* %ptr, i32 6)
  br label %next2
next2:
  %cmp = icmp ult i64 %val64, 1
  br i1 %cmp, label %next, label %next2
}
; CHECK-LABEL: test_local_forward_then_back
; CHECK:      {{.*}} mov DWORD PTR
; CHECK-NEXT: {{.*}} mfence
; CHECK-NEXT: [[LABEL:[0-9a-f]+]]: {{.*}} cmp
; CHECK-NEXT: {{.*}} jb
; CHECK-NEXT: {{.*}} ja
; CHECK-NEXT: {{.*}} cmp
; CHECK-NEXT: {{.*}} jb
; CHECK-NEXT: {{.*}} jmp [[LABEL]]


; Test that backward local branches also work and are small.
; Some of the atomic instructions use a cmpxchg loop.
define internal void @test_local_backward(i64 %val64, i32 %iptr, i32 %val) {
entry:
  br label %next
next:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 5, i32* %ptr, i32 %val, i32 6)
  br label %next2
next2:
  %success = icmp eq i32 1, %a
  br i1 %success, label %next, label %next2
}
; CHECK-LABEL: test_local_backward
; CHECK:       9: {{.*}} mov {{.*}},DWORD
; CHECK:       b: {{.*}} mov
; CHECK-NEXT:  d: {{.*}} xor
; CHECK-NEXT:  f: {{.*}} lock cmpxchg
; CHECK-NEXT: 13: 75 f6 jne b
; CHECK:      1c: 74 eb je 9
