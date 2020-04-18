; Test parsing NaCl atomic instructions.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

declare i8 @llvm.nacl.atomic.load.i8(i8*, i32)
declare i16 @llvm.nacl.atomic.load.i16(i16*, i32)
declare i32 @llvm.nacl.atomic.load.i32(i32*, i32)
declare i64 @llvm.nacl.atomic.load.i64(i64*, i32)
declare void @llvm.nacl.atomic.store.i8(i8, i8*, i32)
declare void @llvm.nacl.atomic.store.i16(i16, i16*, i32)
declare void @llvm.nacl.atomic.store.i32(i32, i32*, i32)
declare void @llvm.nacl.atomic.store.i64(i64, i64*, i32)
declare i8 @llvm.nacl.atomic.rmw.i8(i32, i8*, i8, i32)
declare i16 @llvm.nacl.atomic.rmw.i16(i32, i16*, i16, i32)
declare i32 @llvm.nacl.atomic.rmw.i32(i32, i32*, i32, i32)
declare i64 @llvm.nacl.atomic.rmw.i64(i32, i64*, i64, i32)
declare i8 @llvm.nacl.atomic.cmpxchg.i8(i8*, i8, i8, i32, i32)
declare i16 @llvm.nacl.atomic.cmpxchg.i16(i16*, i16, i16, i32, i32)
declare i32 @llvm.nacl.atomic.cmpxchg.i32(i32*, i32, i32, i32, i32)
declare i64 @llvm.nacl.atomic.cmpxchg.i64(i64*, i64, i64, i32, i32)
declare void @llvm.nacl.atomic.fence(i32)
declare void @llvm.nacl.atomic.fence.all()
declare i1 @llvm.nacl.atomic.is.lock.free(i32, i8*)

;;; Load

define internal i32 @test_atomic_load_8(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  ; parameter value "6" is for the sequential consistency memory order.
  %i = call i8 @llvm.nacl.atomic.load.i8(i8* %ptr, i32 6)
  %r = zext i8 %i to i32
  ret i32 %r
}

; CHECK:      define internal i32 @test_atomic_load_8(i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %i = call i8 @llvm.nacl.atomic.load.i8(i32 %iptr, i32 6)
; CHECK-NEXT:   %r = zext i8 %i to i32
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i32 @test_atomic_load_16(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i16*
  %i = call i16 @llvm.nacl.atomic.load.i16(i16* %ptr, i32 6)
  %r = zext i16 %i to i32
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_atomic_load_16(i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %i = call i16 @llvm.nacl.atomic.load.i16(i32 %iptr, i32 6)
; CHECK-NEXT:   %r = zext i16 %i to i32
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i32 @test_atomic_load_32(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %r = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_atomic_load_32(i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i32 @llvm.nacl.atomic.load.i32(i32 %iptr, i32 6)
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

define internal i64 @test_atomic_load_64(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %r = call i64 @llvm.nacl.atomic.load.i64(i64* %ptr, i32 6)
  ret i64 %r
}

; CHECK-NEXT: define internal i64 @test_atomic_load_64(i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %r = call i64 @llvm.nacl.atomic.load.i64(i32 %iptr, i32 6)
; CHECK-NEXT:   ret i64 %r
; CHECK-NEXT: }

;;; Store

define internal void @test_atomic_store_8(i32 %iptr, i32 %v) {
entry:
  %truncv = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  call void @llvm.nacl.atomic.store.i8(i8 %truncv, i8* %ptr, i32 6)
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_store_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %truncv = trunc i32 %v to i8
; CHECK-NEXT:   call void @llvm.nacl.atomic.store.i8(i8 %truncv, i32 %iptr, i32 6)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_atomic_store_16(i32 %iptr, i32 %v) {
entry:
  %truncv = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  call void @llvm.nacl.atomic.store.i16(i16 %truncv, i16* %ptr, i32 6)
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_store_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %truncv = trunc i32 %v to i16
; CHECK-NEXT:   call void @llvm.nacl.atomic.store.i16(i16 %truncv, i32 %iptr, i32 6)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_atomic_store_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  call void @llvm.nacl.atomic.store.i32(i32 %v, i32* %ptr, i32 6)
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_store_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.nacl.atomic.store.i32(i32 %v, i32 %iptr, i32 6)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_atomic_store_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  call void @llvm.nacl.atomic.store.i64(i64 %v, i64* %ptr, i32 6)
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_store_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.nacl.atomic.store.i64(i64 %v, i32 %iptr, i32 6)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_atomic_store_64_const(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  call void @llvm.nacl.atomic.store.i64(i64 12345678901234, i64* %ptr, i32 6)
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_store_64_const(i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.nacl.atomic.store.i64(i64 12345678901234, i32 %iptr, i32 6)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

;;; RMW

;; add

define internal i32 @test_atomic_rmw_add_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  ; "1" is an atomic add, and "6" is sequential consistency.
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 1, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_add_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i8
; CHECK-NEXT:   %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 1, i32 %iptr, i8 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i8 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_add_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 1, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_add_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i16
; CHECK-NEXT:   %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 1, i32 %iptr, i16 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i16 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_add_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 1, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_add_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 1, i32 %iptr, i32 %v, i32 6)
; CHECK-NEXT:   ret i32 %a
; CHECK-NEXT: }

define internal i64 @test_atomic_rmw_add_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}

; CHECK-NEXT: define internal i64 @test_atomic_rmw_add_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i32 %iptr, i64 %v, i32 6)
; CHECK-NEXT:   ret i64 %a
; CHECK-NEXT: }

;; sub

define internal i32 @test_atomic_rmw_sub_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 2, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_sub_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i8
; CHECK-NEXT:   %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 2, i32 %iptr, i8 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i8 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_sub_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 2, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_sub_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i16
; CHECK-NEXT:   %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 2, i32 %iptr, i16 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i16 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_sub_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 2, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_sub_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 2, i32 %iptr, i32 %v, i32 6)
; CHECK-NEXT:   ret i32 %a
; CHECK-NEXT: }

define internal i64 @test_atomic_rmw_sub_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 2, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}

; CHECK-NEXT: define internal i64 @test_atomic_rmw_sub_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 2, i32 %iptr, i64 %v, i32 6)
; CHECK-NEXT:   ret i64 %a
; CHECK-NEXT: }

;; or

define internal i32 @test_atomic_rmw_or_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 3, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_or_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i8
; CHECK-NEXT:   %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 3, i32 %iptr, i8 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i8 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_or_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 3, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_or_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i16
; CHECK-NEXT:   %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 3, i32 %iptr, i16 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i16 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_or_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 3, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_or_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 3, i32 %iptr, i32 %v, i32 6)
; CHECK-NEXT:   ret i32 %a
; CHECK-NEXT: }

define internal i64 @test_atomic_rmw_or_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 3, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}

; CHECK-NEXT: define internal i64 @test_atomic_rmw_or_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 3, i32 %iptr, i64 %v, i32 6)
; CHECK-NEXT:   ret i64 %a
; CHECK-NEXT: }

;; and

define internal i32 @test_atomic_rmw_and_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 4, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_and_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i8
; CHECK-NEXT:   %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 4, i32 %iptr, i8 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i8 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_and_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 4, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_and_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i16
; CHECK-NEXT:   %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 4, i32 %iptr, i16 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i16 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_and_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 4, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_and_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 4, i32 %iptr, i32 %v, i32 6)
; CHECK-NEXT:   ret i32 %a
; CHECK-NEXT: }

define internal i64 @test_atomic_rmw_and_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 4, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}

; CHECK-NEXT: define internal i64 @test_atomic_rmw_and_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 4, i32 %iptr, i64 %v, i32 6)
; CHECK-NEXT:   ret i64 %a
; CHECK-NEXT: }

;; xor

define internal i32 @test_atomic_rmw_xor_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 5, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_xor_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i8
; CHECK-NEXT:   %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 5, i32 %iptr, i8 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i8 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_xor_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 5, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_xor_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i16
; CHECK-NEXT:   %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 5, i32 %iptr, i16 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i16 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_xor_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 5, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_xor_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 5, i32 %iptr, i32 %v, i32 6)
; CHECK-NEXT:   ret i32 %a
; CHECK-NEXT: }

define internal i64 @test_atomic_rmw_xor_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 5, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}

; CHECK-NEXT: define internal i64 @test_atomic_rmw_xor_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 5, i32 %iptr, i64 %v, i32 6)
; CHECK-NEXT:   ret i64 %a
; CHECK-NEXT: }

;; exchange

define internal i32 @test_atomic_rmw_xchg_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 6, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_xchg_8(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i8
; CHECK-NEXT:   %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 6, i32 %iptr, i8 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i8 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_xchg_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 6, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_xchg_16(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc = trunc i32 %v to i16
; CHECK-NEXT:   %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 6, i32 %iptr, i16 %trunc, i32 6)
; CHECK-NEXT:   %a_ext = zext i16 %a to i32
; CHECK-NEXT:   ret i32 %a_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_rmw_xchg_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 6, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}

; CHECK-NEXT: define internal i32 @test_atomic_rmw_xchg_32(i32 %iptr, i32 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 6, i32 %iptr, i32 %v, i32 6)
; CHECK-NEXT:   ret i32 %a
; CHECK-NEXT: }

define internal i64 @test_atomic_rmw_xchg_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 6, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}

; CHECK-NEXT: define internal i64 @test_atomic_rmw_xchg_64(i32 %iptr, i64 %v) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 6, i32 %iptr, i64 %v, i32 6)
; CHECK-NEXT:   ret i64 %a
; CHECK-NEXT: }

;;;; Cmpxchg

define internal i32 @test_atomic_cmpxchg_8(i32 %iptr, i32 %expected, i32 %desired) {
entry:
  %trunc_exp = trunc i32 %expected to i8
  %trunc_des = trunc i32 %desired to i8
  %ptr = inttoptr i32 %iptr to i8*
  %old = call i8 @llvm.nacl.atomic.cmpxchg.i8(i8* %ptr, i8 %trunc_exp,
                                              i8 %trunc_des, i32 6, i32 6)
  %old_ext = zext i8 %old to i32
  ret i32 %old_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_cmpxchg_8(i32 %iptr, i32 %expected, i32 %desired) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc_exp = trunc i32 %expected to i8
; CHECK-NEXT:   %trunc_des = trunc i32 %desired to i8
; CHECK-NEXT:   %old = call i8 @llvm.nacl.atomic.cmpxchg.i8(i32 %iptr, i8 %trunc_exp, i8 %trunc_des, i32 6, i32 6)
; CHECK-NEXT:   %old_ext = zext i8 %old to i32
; CHECK-NEXT:   ret i32 %old_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_cmpxchg_16(i32 %iptr, i32 %expected, i32 %desired) {
entry:
  %trunc_exp = trunc i32 %expected to i16
  %trunc_des = trunc i32 %desired to i16
  %ptr = inttoptr i32 %iptr to i16*
  %old = call i16 @llvm.nacl.atomic.cmpxchg.i16(i16* %ptr, i16 %trunc_exp,
                                               i16 %trunc_des, i32 6, i32 6)
  %old_ext = zext i16 %old to i32
  ret i32 %old_ext
}

; CHECK-NEXT: define internal i32 @test_atomic_cmpxchg_16(i32 %iptr, i32 %expected, i32 %desired) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %trunc_exp = trunc i32 %expected to i16
; CHECK-NEXT:   %trunc_des = trunc i32 %desired to i16
; CHECK-NEXT:   %old = call i16 @llvm.nacl.atomic.cmpxchg.i16(i32 %iptr, i16 %trunc_exp, i16 %trunc_des, i32 6, i32 6)
; CHECK-NEXT:   %old_ext = zext i16 %old to i32
; CHECK-NEXT:   ret i32 %old_ext
; CHECK-NEXT: }

define internal i32 @test_atomic_cmpxchg_32(i32 %iptr, i32 %expected, i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                               i32 %desired, i32 6, i32 6)
  ret i32 %old
}

; CHECK-NEXT: define internal i32 @test_atomic_cmpxchg_32(i32 %iptr, i32 %expected, i32 %desired) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32 %iptr, i32 %expected, i32 %desired, i32 6, i32 6)
; CHECK-NEXT:   ret i32 %old
; CHECK-NEXT: }

define internal i64 @test_atomic_cmpxchg_64(i32 %iptr, i64 %expected, i64 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 %expected,
                                               i64 %desired, i32 6, i32 6)
  ret i64 %old
}

; CHECK-NEXT: define internal i64 @test_atomic_cmpxchg_64(i32 %iptr, i64 %expected, i64 %desired) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i32 %iptr, i64 %expected, i64 %desired, i32 6, i32 6)
; CHECK-NEXT:   ret i64 %old
; CHECK-NEXT: }

;;;; Fence and is-lock-free.

define internal void @test_atomic_fence() {
entry:
  call void @llvm.nacl.atomic.fence(i32 6)
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_fence() {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.nacl.atomic.fence(i32 6)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal void @test_atomic_fence_all() {
entry:
  call void @llvm.nacl.atomic.fence.all()
  ret void
}

; CHECK-NEXT: define internal void @test_atomic_fence_all() {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @llvm.nacl.atomic.fence.all()
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define internal i32 @test_atomic_is_lock_free(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i1 @llvm.nacl.atomic.is.lock.free(i32 4, i8* %ptr)
  %r = zext i1 %i to i32
  ret i32 %r
}

; CHECK-NEXT: define internal i32 @test_atomic_is_lock_free(i32 %iptr) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %i = call i1 @llvm.nacl.atomic.is.lock.free(i32 4, i32 %iptr)
; CHECK-NEXT:   %r = zext i1 %i to i32
; CHECK-NEXT:   ret i32 %r
; CHECK-NEXT: }

; NOIR: Total across all functions
