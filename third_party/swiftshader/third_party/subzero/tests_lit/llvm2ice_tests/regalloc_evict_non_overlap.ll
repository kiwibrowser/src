; Bugpoint-reduced example that demonstrated a bug (assertion failure)
; in register allocation.  See
; https://code.google.com/p/nativeclient/issues/detail?id=3903 .
;
; TODO(kschimpf) Find out why lc2i is needed.
; RUN: %lc2i -i %s --args -O2 --verbose regalloc

define internal void @foo() {
bb:
  br i1 undef, label %bb13, label %bb14

bb13:
  unreachable

bb14:
  br i1 undef, label %bb50, label %bb16

bb15:                                             ; preds = %bb42, %bb35
  br i1 undef, label %bb50, label %bb16

bb16:                                             ; preds = %bb49, %bb15, %bb14
  %tmp = phi i32 [ undef, %bb14 ], [ %tmp18, %bb49 ], [ undef, %bb15 ]
  br label %bb17

bb17:                                             ; preds = %bb48, %bb16
  %tmp18 = phi i32 [ undef, %bb16 ], [ undef, %bb48 ]
  %tmp19 = add i32 %tmp18, 4
  br i1 undef, label %bb21, label %bb46

bb21:                                             ; preds = %bb27, %bb17
  %tmp22 = phi i32 [ undef, %bb17 ], [ %tmp30, %bb27 ]
  %tmp23 = add i32 undef, -1
  %tmp24 = add i32 undef, undef
  %undef.ptr = inttoptr i32 undef to i32*
  %tmp25 = load i32, i32* %undef.ptr, align 1
  %tmp26 = icmp eq i32 undef, %tmp22
  br i1 %tmp26, label %bb34, label %bb32

bb27:                                             ; preds = %bb42, %bb34
  %tmp28 = icmp sgt i32 %tmp23, 0
  %tmp29 = inttoptr i32 %tmp19 to i32*
  %tmp30 = load i32, i32* %tmp29, align 1
  br i1 %tmp28, label %bb21, label %bb46

bb32:                                             ; preds = %bb21
  %tmp33 = inttoptr i32 %tmp24 to i32*
  store i32 0, i32* %tmp33, align 1
  br label %bb34

bb34:                                             ; preds = %bb32, %bb31
  br i1 undef, label %bb27, label %bb35

bb35:                                             ; preds = %bb34
  %tmp40 = inttoptr i32 %tmp25 to void (i32)*
  call void %tmp40(i32 undef)
  br i1 undef, label %bb42, label %bb15

bb42:                                             ; preds = %bb35
  %tmp43 = inttoptr i32 %tmp to i32*
  %tmp44 = load i32, i32* %tmp43, align 1
  %tmp45 = icmp eq i32 %tmp44, %tmp18
  br i1 %tmp45, label %bb27, label %bb15

bb46:                                             ; preds = %bb27, %bb17
  br i1 undef, label %bb47, label %bb49

bb47:                                             ; preds = %bb46
  br i1 undef, label %bb50, label %bb48

bb48:                                             ; preds = %bb47
  br i1 undef, label %bb50, label %bb17

bb49:                                             ; preds = %bb46
  br i1 undef, label %bb50, label %bb16

bb50:                                             ; preds = %bb49, %bb48, %bb47, %bb15, %bb14
  unreachable
}
