; Test that some errors trigger when the usage of NaCl atomic
; intrinsics does not match the required ABI.
; REQUIRES: allow_dump

; RUN: %p2i -i %s --args --verbose none --exit-success -threads=0 2>&1 \
; RUN:   | FileCheck %s

declare i8 @llvm.nacl.atomic.load.i8(i8*, i32)
declare i16 @llvm.nacl.atomic.load.i16(i16*, i32)
declare i64 @llvm.nacl.atomic.load.i64(i64*, i32)
declare void @llvm.nacl.atomic.store.i32(i32, i32*, i32)
declare void @llvm.nacl.atomic.store.i64(i64, i64*, i32)
declare i8 @llvm.nacl.atomic.rmw.i8(i32, i8*, i8, i32)
declare i16 @llvm.nacl.atomic.rmw.i16(i32, i16*, i16, i32)
declare i32 @llvm.nacl.atomic.rmw.i32(i32, i32*, i32, i32)
declare i64 @llvm.nacl.atomic.rmw.i64(i32, i64*, i64, i32)
declare i32 @llvm.nacl.atomic.cmpxchg.i32(i32*, i32, i32, i32, i32)
declare i64 @llvm.nacl.atomic.cmpxchg.i64(i64*, i64, i64, i32, i32)
declare void @llvm.nacl.atomic.fence(i32)
declare i1 @llvm.nacl.atomic.is.lock.free(i32, i8*)

;;; Load
;;; Check unexpected memory order parameter (release=4 and acq_rel=5
;;; are disallowed).

define internal i32 @error_atomic_load_8(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i8 @llvm.nacl.atomic.load.i8(i8* %ptr, i32 0)
  %r = zext i8 %i to i32
  ret i32 %r
}
; CHECK: Unexpected memory ordering for AtomicLoad

define internal i32 @error_atomic_load_16(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i16*
  %i = call i16 @llvm.nacl.atomic.load.i16(i16* %ptr, i32 4)
  %r = zext i16 %i to i32
  ret i32 %r
}
; CHECK: Unexpected memory ordering for AtomicLoad

define internal i64 @error_atomic_load_64(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %r = call i64 @llvm.nacl.atomic.load.i64(i64* %ptr, i32 5)
  ret i64 %r
}
; CHECK: Unexpected memory ordering for AtomicLoad


;;; Store
;;; consume=2, acquire=3, acq_rel=5 are disallowed

define internal void @error_atomic_store_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  call void @llvm.nacl.atomic.store.i32(i32 %v, i32* %ptr, i32 2)
  ret void
}
; CHECK: Unexpected memory ordering for AtomicStore

define internal void @error_atomic_store_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  call void @llvm.nacl.atomic.store.i64(i64 %v, i64* %ptr, i32 3)
  ret void
}
; CHECK: Unexpected memory ordering for AtomicStore

define internal void @error_atomic_store_64_const(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  call void @llvm.nacl.atomic.store.i64(i64 12345678901234, i64* %ptr, i32 5)
  ret void
}
; CHECK: Unexpected memory ordering for AtomicStore

;;; RMW
;;; Test atomic memory order and operation.
;;; Modes 3:6 allowed.

define internal i32 @error_atomic_rmw_add_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 1, i8* %ptr, i8 %trunc, i32 1)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK: Unexpected memory ordering for AtomicRMW

define internal i64 @error_atomic_rmw_add_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr, i64 %v, i32 7)
  ret i64 %a
}
; CHECK: Unexpected memory ordering for AtomicRMW

define internal i32 @error_atomic_rmw_add_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 0, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK: Unknown AtomicRMW operation

define internal i32 @error_atomic_rmw_add_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 7, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK: Unknown AtomicRMW operation

define internal i32 @error_atomic_rmw_add_32_max(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 4294967295, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK: Unknown AtomicRMW operation

;;; Cmpxchg

define internal i32 @error_atomic_cmpxchg_32_success(i32 %iptr, i32 %expected,
                                                     i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                               i32 %desired, i32 0, i32 6)
  ret i32 %old
}
; CHECK: Unexpected memory ordering for AtomicCmpxchg

define internal i32 @error_atomic_cmpxchg_32_failure(i32 %iptr, i32 %expected,
                                                     i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                               i32 %desired, i32 6, i32 0)
  ret i32 %old
}
; CHECK: Unexpected memory ordering for AtomicCmpxchg

define internal i64 @error_atomic_cmpxchg_64_failure(i32 %iptr, i64 %expected,
                                                     i64 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 %expected,
                                               i64 %desired, i32 4, i32 1)
  ret i64 %old
}
; CHECK: Unexpected memory ordering for AtomicCmpxchg

;;; Fence and is-lock-free.

define internal void @error_atomic_fence() {
entry:
  call void @llvm.nacl.atomic.fence(i32 0)
  ret void
}
; CHECK: Unexpected memory ordering for AtomicFence

define internal i32 @error_atomic_is_lock_free_var(i32 %iptr, i32 %bs) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i1 @llvm.nacl.atomic.is.lock.free(i32 %bs, i8* %ptr)
  %r = zext i1 %i to i32
  ret i32 %r
}
; CHECK: AtomicIsLockFree byte size should be compile-time const


;;; Test bad non-constant memory ordering values.

define internal i32 @error_atomic_load_8_nonconst(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i8 @llvm.nacl.atomic.load.i8(i8* %ptr, i32 %iptr)
  %r = zext i8 %i to i32
  ret i32 %r
}
; CHECK: Unexpected memory ordering for AtomicLoad

define internal void @error_atomic_store_32_nonconst(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  call void @llvm.nacl.atomic.store.i32(i32 %v, i32* %ptr, i32 %v)
  ret void
}
; CHECK: Unexpected memory ordering for AtomicStore

define internal i32 @error_atomic_rmw_add_8_nonconst(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 1, i8* %ptr, i8 %trunc, i32 %iptr)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK: Unexpected memory ordering for AtomicRMW

define internal i32 @error_atomic_cmpxchg_32_success_nonconst_1(i32 %iptr, i32 %expected,
                                                                i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                               i32 %desired, i32 %iptr, i32 6)
  ret i32 %old
}
; CHECK: Unexpected memory ordering for AtomicCmpxchg

define internal i32 @error_atomic_cmpxchg_32_success_nonconst_2(i32 %iptr, i32 %expected,
                                                                i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                               i32 %desired, i32 6, i32 %iptr)
  ret i32 %old
}
; CHECK: Unexpected memory ordering for AtomicCmpxchg

define internal void @error_atomic_fence_nonconst(i32 %v) {
entry:
  call void @llvm.nacl.atomic.fence(i32 %v)
  ret void
}
; CHECK: Unexpected memory ordering for AtomicFence
