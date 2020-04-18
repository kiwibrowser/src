; This tests each of the supported NaCl atomic instructions for every
; size allowed.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:   -allow-externally-defined-symbols | FileCheck --check-prefix=O2 %s
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

; RUN: %if --need=allow_dump --need=target_ARM32 --command %p2i --filetype=asm \
; RUN:   --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32

; RUN: %if --need=allow_dump --need=target_ARM32 --command %p2i --filetype=asm \
; RUN:   --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32O2

; RUN: %if --need=allow_dump --need=target_ARM32 --command %p2i --filetype=asm \
; RUN:   --target arm32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_ARM32 --command FileCheck %s \
; RUN:   --check-prefix=ARM32

; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i --filetype=asm\
; RUN:   --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=MIPS32O2 --check-prefix=MIPS32

; RUN: %if --need=allow_dump --need=target_MIPS32 --command %p2i --filetype=asm\
; RUN:   --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --need=target_MIPS32 --command FileCheck %s \
; RUN:   --check-prefix=MIPS32OM1 --check-prefix=MIPS32

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

@SzGlobal8 = internal global [1 x i8] zeroinitializer, align 1
@SzGlobal16 = internal global [2 x i8] zeroinitializer, align 2
@SzGlobal32 = internal global [4 x i8] zeroinitializer, align 4
@SzGlobal64 = internal global [8 x i8] zeroinitializer, align 8

; NOTE: The LLC equivalent for 16-bit atomic operations are expanded
; as 32-bit operations. For Subzero, assume that real 16-bit operations
; will be usable (the validator will be fixed):
; https://code.google.com/p/nativeclient/issues/detail?id=2981

;;; Load

; x86 guarantees load/store to be atomic if naturally aligned.
; The PNaCl IR requires all atomic accesses to be naturally aligned.

define internal i32 @test_atomic_load_8(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  ; parameter value "6" is for the sequential consistency memory order.
  %i = call i8 @llvm.nacl.atomic.load.i8(i8* %ptr, i32 6)
  %i2 = sub i8 %i, 0
  %r = zext i8 %i2 to i32
  ret i32 %r
}
; CHECK-LABEL: test_atomic_load_8
; CHECK: mov {{.*}},DWORD
; CHECK: mov {{.*}},BYTE
; ARM32-LABEL: test_atomic_load_8
; ARM32: ldrb r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_8
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal i32 @test_atomic_load_16(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i16*
  %i = call i16 @llvm.nacl.atomic.load.i16(i16* %ptr, i32 6)
  %i2 = sub i16 %i, 0
  %r = zext i16 %i2 to i32
  ret i32 %r
}
; CHECK-LABEL: test_atomic_load_16
; CHECK: mov {{.*}},DWORD
; CHECK: mov {{.*}},WORD
; ARM32-LABEL: test_atomic_load_16
; ARM32: ldrh r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_16
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal i32 @test_atomic_load_32(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %r = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  ret i32 %r
}
; CHECK-LABEL: test_atomic_load_32
; CHECK: mov {{.*}},DWORD
; CHECK: mov {{.*}},DWORD
; ARM32-LABEL: test_atomic_load_32
; ARM32: ldr r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal i64 @test_atomic_load_64(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %r = call i64 @llvm.nacl.atomic.load.i64(i64* %ptr, i32 6)
  ret i64 %r
}
; CHECK-LABEL: test_atomic_load_64
; CHECK: movq x{{.*}},QWORD
; CHECK: movq QWORD {{.*}},x{{.*}}
; ARM32-LABEL: test_atomic_load_64
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_64
; MIPS32: jal __sync_val_compare_and_swap_8
; MIPS32: sync

define internal i32 @test_atomic_load_32_with_arith(i32 %iptr) {
entry:
  br label %next

next:
  %ptr = inttoptr i32 %iptr to i32*
  %r = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  %r2 = sub i32 32, %r
  ret i32 %r2
}
; CHECK-LABEL: test_atomic_load_32_with_arith
; CHECK: mov {{.*}},DWORD
; The next instruction may be a separate load or folded into an add.
;
; In O2 mode, we know that the load and sub are going to be fused.
; O2-LABEL: test_atomic_load_32_with_arith
; O2: mov {{.*}},DWORD
; O2: sub {{.*}},DWORD
; ARM32-LABEL: test_atomic_load_32_with_arith
; ARM32: ldr r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_32_with_arith
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync
; MIPS32: subu

define internal i32 @test_atomic_load_32_ignored(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.load.i32(i32* %ptr, i32 6)
  ret i32 0
}
; CHECK-LABEL: test_atomic_load_32_ignored
; CHECK: mov {{.*}},DWORD
; CHECK: mov {{.*}},DWORD
; O2-LABEL: test_atomic_load_32_ignored
; O2: mov {{.*}},DWORD
; O2: mov {{.*}},DWORD
; ARM32-LABEL: test_atomic_load_32_ignored
; ARM32: ldr r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal i64 @test_atomic_load_64_ignored(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %ignored = call i64 @llvm.nacl.atomic.load.i64(i64* %ptr, i32 6)
  ret i64 0
}
; CHECK-LABEL: test_atomic_load_64_ignored
; CHECK: movq x{{.*}},QWORD
; CHECK: movq QWORD {{.*}},x{{.*}}
; ARM32-LABEL: test_atomic_load_64_ignored
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_load_64_ignored
; MIPS32: jal	__sync_val_compare_and_swap_8
; MIPS32: sync

;;; Store

define internal void @test_atomic_store_8(i32 %iptr, i32 %v) {
entry:
  %truncv = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  call void @llvm.nacl.atomic.store.i8(i8 %truncv, i8* %ptr, i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_store_8
; CHECK: mov BYTE
; CHECK: mfence
; ARM32-LABEL: test_atomic_store_8
; ARM32: dmb
; ARM32: strb r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_store_8
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal void @test_atomic_store_16(i32 %iptr, i32 %v) {
entry:
  %truncv = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  call void @llvm.nacl.atomic.store.i16(i16 %truncv, i16* %ptr, i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_store_16
; CHECK: mov WORD
; CHECK: mfence
; ARM32-LABEL: test_atomic_store_16
; ARM32: dmb
; ARM32: strh r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_store_16
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal void @test_atomic_store_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  call void @llvm.nacl.atomic.store.i32(i32 %v, i32* %ptr, i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_store_32
; CHECK: mov DWORD
; CHECK: mfence
; ARM32-LABEL: test_atomic_store_32
; ARM32: dmb
; ARM32: str r{{[0-9]+}}, [r{{[0-9]+}}
; ARM32: dmb
; MIPS32-LABEL: test_atomic_store_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: sc
; MIPS32: sync

define internal void @test_atomic_store_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  call void @llvm.nacl.atomic.store.i64(i64 %v, i64* %ptr, i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_store_64
; CHECK: movq x{{.*}},QWORD
; CHECK: movq QWORD {{.*}},x{{.*}}
; CHECK: mfence
; ARM32-LABEL: test_atomic_store_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [[MEM:.*]]
; ARM32: strexd [[S:r[0-9]+]], r{{[0-9]+}}, r{{[0-9]+}}, [[MEM]]
; ARM32: cmp [[S]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_store_64
; MIPS32: sync
; MIPS32: jal	__sync_lock_test_and_set_8
; MIPS32: sync

define internal void @test_atomic_store_64_const(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  call void @llvm.nacl.atomic.store.i64(i64 12345678901234, i64* %ptr, i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_store_64_const
; CHECK: mov {{.*}},0x73ce2ff2
; CHECK: mov {{.*}},0xb3a
; CHECK: movq x{{.*}},QWORD
; CHECK: movq QWORD {{.*}},x{{.*}}
; CHECK: mfence
; ARM32-LABEL: test_atomic_store_64_const
; ARM32: movw [[T0:r[0-9]+]], #12274
; ARM32: movt [[T0]], #29646
; ARM32: movw r{{[0-9]+}}, #2874
; ARM32: dmb
; ARM32: .L[[RETRY:.*]]:
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [[MEM:.*]]
; ARM32: strexd [[S:r[0-9]+]], r{{[0-9]+}}, r{{[0-9]+}}, [[MEM]]
; ARM32: cmp [[S]], #0
; ARM32: bne .L[[RETRY]]
; ARM32: dmb
; MIPS32-LABEL: test_atomic_store_64_const
; MIPS32: sync
; MIPS32: lui	{{.*}}, 29646
; MIPS32: ori	{{.*}},{{.*}}, 12274
; MIPS32: addiu	{{.*}}, $zero, 2874
; MIPS32: jal	__sync_lock_test_and_set_8
; MIPS32: sync

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
; CHECK-LABEL: test_atomic_rmw_add_8
; CHECK: lock xadd BYTE {{.*}},[[REG:.*]]
; CHECK: {{mov|movzx}} {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_add_8
; ARM32: dmb
; ARM32: ldrexb
; ARM32: add
; ARM32: strexb
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_8
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: addu
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_rmw_add_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 1, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_add_16
; CHECK: lock xadd WORD {{.*}},[[REG:.*]]
; CHECK: {{mov|movzx}} {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_add_16
; ARM32: dmb
; ARM32: ldrexh
; ARM32: add
; ARM32: strexh
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_16
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: addu
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_rmw_add_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 1, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_add_32
; CHECK: lock xadd DWORD {{.*}},[[REG:.*]]
; CHECK: mov {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_add_32
; ARM32: dmb
; ARM32: ldrex
; ARM32: add
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: addu
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_rmw_add_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_add_64
; CHECK: push ebx
; CHECK: mov eax,DWORD PTR [{{.*}}]
; CHECK: mov edx,DWORD PTR [{{.*}}+0x4]
; CHECK: [[LABEL:[^ ]*]]: {{.*}} mov ebx,eax
; RHS of add cannot be any of the e[abcd]x regs because they are
; clobbered in the loop, and the RHS needs to be remain live.
; CHECK: add ebx,{{.*e.[^x]}}
; CHECK: mov ecx,edx
; CHECK: adc ecx,{{.*e.[^x]}}
; Ptr cannot be eax, ebx, ecx, or edx (used up for the expected and desired).
; It can be esi, edi, or ebp though, for example (so we need to be careful
; about rejecting eb* and ed*.)
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}
; CHECK: jne [[LABEL]]
; ARM32-LABEL: test_atomic_rmw_add_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: adds
; ARM32: adc
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_64
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_add_8
; MIPS32: sync

; Same test as above, but with a global address to test FakeUse issues.
define internal i64 @test_atomic_rmw_add_64_global(i64 %v) {
entry:
  %ptr = bitcast [8 x i8]* @SzGlobal64 to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_add_64_global
; ARM32-LABEL: test_atomic_rmw_add_64_global
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: adds
; ARM32: adc
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_64_global
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_add_8
; MIPS32: sync

; Test with some more register pressure. When we have an alloca, ebp is
; used to manage the stack frame, so it cannot be used as a register either.
declare void @use_ptr(i32 %iptr)

define internal i64 @test_atomic_rmw_add_64_alloca(i32 %iptr, i64 %v) {
entry:
  br label %eblock  ; Disable alloca optimization
eblock:
  %alloca_ptr = alloca i8, i32 16, align 16
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr, i64 %v, i32 6)
  store i8 0, i8* %alloca_ptr, align 1
  store i8 1, i8* %alloca_ptr, align 1
  store i8 2, i8* %alloca_ptr, align 1
  store i8 3, i8* %alloca_ptr, align 1
  %__5 = ptrtoint i8* %alloca_ptr to i32
  call void @use_ptr(i32 %__5)
  ret i64 %old
}
; CHECK-LABEL: test_atomic_rmw_add_64_alloca
; CHECK: push ebx
; CHECK-DAG: mov edx
; CHECK-DAG: mov eax
; CHECK-DAG: mov ecx
; CHECK-DAG: mov ebx
; Ptr cannot be eax, ebx, ecx, or edx (used up for the expected and desired).
; It also cannot be ebp since we use that for alloca. Also make sure it's
; not esp, since that's the stack pointer and mucking with it will break
; the later use_ptr function call.
; That pretty much leaves esi, or edi as the only viable registers.
; CHECK: lock cmpxchg8b QWORD PTR [e{{[ds]}}i]
; CHECK: call {{.*}} R_{{.*}} use_ptr
; ARM32-LABEL: test_atomic_rmw_add_64_alloca
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: adds
; ARM32: adc
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_64_alloca
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_add_8
; MIPS32: sync

define internal i32 @test_atomic_rmw_add_32_ignored(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.rmw.i32(i32 1, i32* %ptr, i32 %v, i32 6)
  ret i32 %v
}
; Technically this could use "lock add" instead of "lock xadd", if liveness
; tells us that the destination variable is dead.
; CHECK-LABEL: test_atomic_rmw_add_32_ignored
; CHECK: lock xadd DWORD {{.*}},[[REG:.*]]
; ARM32-LABEL: test_atomic_rmw_add_32_ignored
; ARM32: dmb
; ARM32: ldrex
; ARM32: add
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_add_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: addu
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

; Atomic RMW 64 needs to be expanded into its own loop.
; Make sure that works w/ non-trivial function bodies.
define internal i64 @test_atomic_rmw_add_64_loop(i32 %iptr, i64 %v) {
entry:
  %x = icmp ult i64 %v, 100
  br i1 %x, label %err, label %loop

loop:
  %v_next = phi i64 [ %v, %entry ], [ %next, %loop ]
  %ptr = inttoptr i32 %iptr to i64*
  %next = call i64 @llvm.nacl.atomic.rmw.i64(i32 1, i64* %ptr, i64 %v_next, i32 6)
  %success = icmp eq i64 %next, 100
  br i1 %success, label %done, label %loop

done:
  ret i64 %next

err:
  ret i64 0
}
; CHECK-LABEL: test_atomic_rmw_add_64_loop
; CHECK: push ebx
; CHECK: mov eax,DWORD PTR [{{.*}}]
; CHECK: mov edx,DWORD PTR [{{.*}}+0x4]
; CHECK: [[LABEL:[^ ]*]]: {{.*}} mov ebx,eax
; CHECK: add ebx,{{.*e.[^x]}}
; CHECK: mov ecx,edx
; CHECK: adc ecx,{{.*e.[^x]}}
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}+0x0]
; CHECK: jne [[LABEL]]
; ARM32-LABEL: test_atomic_rmw_add_64_loop
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: adds
; ARM32: adc
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; ARM32: b
; MIPS32-LABEL: test_atomic_rmw_add_64_loop
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_add_8
; MIPS32: sync

;; sub

define internal i32 @test_atomic_rmw_sub_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 2, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_sub_8
; CHECK: neg [[REG:.*]]
; CHECK: lock xadd BYTE {{.*}},[[REG]]
; CHECK: {{mov|movzx}} {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_sub_8
; ARM32: dmb
; ARM32: ldrexb
; ARM32: sub
; ARM32: strexb
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_sub_8
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: subu
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_rmw_sub_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 2, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_sub_16
; CHECK: neg [[REG:.*]]
; CHECK: lock xadd WORD {{.*}},[[REG]]
; CHECK: {{mov|movzx}} {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_sub_16
; ARM32: dmb
; ARM32: ldrexh
; ARM32: sub
; ARM32: strexh
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_sub_16
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: subu
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_rmw_sub_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 2, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_sub_32
; CHECK: neg [[REG:.*]]
; CHECK: lock xadd DWORD {{.*}},[[REG]]
; CHECK: mov {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_sub_32
; ARM32: dmb
; ARM32: ldrex
; ARM32: sub
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_sub_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: subu
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_rmw_sub_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 2, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_sub_64
; CHECK: push ebx
; CHECK: mov eax,DWORD PTR [{{.*}}]
; CHECK: mov edx,DWORD PTR [{{.*}}+0x4]
; CHECK: [[LABEL:[^ ]*]]: {{.*}} mov ebx,eax
; CHECK: sub ebx,{{.*e.[^x]}}
; CHECK: mov ecx,edx
; CHECK: sbb ecx,{{.*e.[^x]}}
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}
; CHECK: jne [[LABEL]]
; ARM32-LABEL: test_atomic_rmw_sub_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: subs
; ARM32: sbc
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_sub_64
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_sub_8
; MIPS32: sync

define internal i32 @test_atomic_rmw_sub_32_ignored(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.rmw.i32(i32 2, i32* %ptr, i32 %v, i32 6)
  ret i32 %v
}
; Could use "lock sub" instead of "neg; lock xadd"
; CHECK-LABEL: test_atomic_rmw_sub_32_ignored
; CHECK: neg [[REG:.*]]
; CHECK: lock xadd DWORD {{.*}},[[REG]]
; ARM32-LABEL: test_atomic_rmw_sub_32_ignored
; ARM32: dmb
; ARM32: ldrex
; ARM32: sub
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_sub_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: subu
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

;; or

define internal i32 @test_atomic_rmw_or_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 3, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_or_8
; CHECK: mov al,BYTE PTR
; Dest cannot be eax here, because eax is used for the old value. Also want
; to make sure that cmpxchg's source is the same register.
; CHECK: or [[REG:[^a].]]
; CHECK: lock cmpxchg BYTE PTR [e{{[^a].}}],[[REG]]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_or_8
; ARM32: dmb
; ARM32: ldrexb
; ARM32: orr
; ARM32: strexb
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_8
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: or
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

; Same test as above, but with a global address to test FakeUse issues.
define internal i32 @test_atomic_rmw_or_8_global(i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = bitcast [1 x i8]* @SzGlobal8 to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 3, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_or_8_global
; ARM32-LABEL: test_atomic_rmw_or_8_global
; ARM32: dmb
; ARM32: movw [[PTR:r[0-9]+]], #:lower16:SzGlobal8
; ARM32: movt [[PTR]], #:upper16:SzGlobal8
; ARM32: ldrexb r{{[0-9]+}}, {{[[]}}[[PTR]]{{[]]}}
; ARM32: orr
; ARM32: strexb
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_8_global
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: or
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_rmw_or_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 3, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_or_16
; CHECK: mov ax,WORD PTR
; CHECK: or [[REG:[^a].]]
; CHECK: lock cmpxchg WORD PTR [e{{[^a].}}],[[REG]]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_or_16
; ARM32: dmb
; ARM32: ldrexh
; ARM32: orr
; ARM32: strexh
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_16
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: or
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

; Same test as above, but with a global address to test FakeUse issues.
define internal i32 @test_atomic_rmw_or_16_global(i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = bitcast [2 x i8]* @SzGlobal16 to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 3, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_or_16_global
; ARM32-LABEL: test_atomic_rmw_or_16_global
; ARM32: dmb
; ARM32: movw [[PTR:r[0-9]+]], #:lower16:SzGlobal16
; ARM32: movt [[PTR]], #:upper16:SzGlobal16
; ARM32: ldrexh r{{[0-9]+}}, {{[[]}}[[PTR]]{{[]]}}
; ARM32: orr
; ARM32: strexh
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_16_global
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: or
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_rmw_or_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 3, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_or_32
; CHECK: mov eax,DWORD PTR
; CHECK: or [[REG:e[^a].]]
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}],[[REG]]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_or_32
; ARM32: dmb
; ARM32: ldrex
; ARM32: orr
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

; Same test as above, but with a global address to test FakeUse issues.
define internal i32 @test_atomic_rmw_or_32_global(i32 %v) {
entry:
  %ptr = bitcast [4 x i8]* @SzGlobal32 to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 3, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_or_32_global
; ARM32-LABEL: test_atomic_rmw_or_32_global
; ARM32: dmb
; ARM32: movw [[PTR:r[0-9]+]], #:lower16:SzGlobal32
; ARM32: movt [[PTR]], #:upper16:SzGlobal32
; ARM32: ldrex r{{[0-9]+}}, {{[[]}}[[PTR]]{{[]]}}
; ARM32: orr
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_32_global
; MIPS32: sync
; MIPS32: ll
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_rmw_or_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 3, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_or_64
; CHECK: push ebx
; CHECK: mov eax,DWORD PTR [{{.*}}]
; CHECK: mov edx,DWORD PTR [{{.*}}+0x4]
; CHECK: [[LABEL:[^ ]*]]: {{.*}} mov ebx,eax
; CHECK: or ebx,{{.*e.[^x]}}
; CHECK: mov ecx,edx
; CHECK: or ecx,{{.*e.[^x]}}
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}
; CHECK: jne [[LABEL]]
; ARM32-LABEL: test_atomic_rmw_or_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: orr
; ARM32: orr
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_64
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_or_8
; MIPS32: sync

define internal i32 @test_atomic_rmw_or_32_ignored(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.rmw.i32(i32 3, i32* %ptr, i32 %v, i32 6)
  ret i32 %v
}
; CHECK-LABEL: test_atomic_rmw_or_32_ignored
; Could just "lock or", if we inspect the liveness information first.
; Would also need a way to introduce "lock"'edness to binary
; operators without introducing overhead on the more common binary ops.
; CHECK: mov eax,DWORD PTR
; CHECK: or [[REG:e[^a].]]
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}],[[REG]]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_or_32_ignored
; ARM32: dmb
; ARM32: ldrex
; ARM32: orr
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_or_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

;; and

define internal i32 @test_atomic_rmw_and_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 4, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_and_8
; CHECK: mov al,BYTE PTR
; CHECK: and [[REG:[^a].]]
; CHECK: lock cmpxchg BYTE PTR [e{{[^a].}}],[[REG]]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_and_8
; ARM32: dmb
; ARM32: ldrexb
; ARM32: and
; ARM32: strexb
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_and_8
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: and
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_rmw_and_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 4, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_and_16
; CHECK: mov ax,WORD PTR
; CHECK: and
; CHECK: lock cmpxchg WORD PTR [e{{[^a].}}]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_and_16
; ARM32: dmb
; ARM32: ldrexh
; ARM32: and
; ARM32: strexh
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_and_16
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: and
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_rmw_and_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 4, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_and_32
; CHECK: mov eax,DWORD PTR
; CHECK: and
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_and_32
; ARM32: dmb
; ARM32: ldrex
; ARM32: and
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_and_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: and
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_rmw_and_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 4, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_and_64
; CHECK: push ebx
; CHECK: mov eax,DWORD PTR [{{.*}}]
; CHECK: mov edx,DWORD PTR [{{.*}}+0x4]
; CHECK: [[LABEL:[^ ]*]]: {{.*}} mov ebx,eax
; CHECK: and ebx,{{.*e.[^x]}}
; CHECK: mov ecx,edx
; CHECK: and ecx,{{.*e.[^x]}}
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}
; CHECK: jne [[LABEL]]
; ARM32-LABEL: test_atomic_rmw_and_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: and
; ARM32: and
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_and_64
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_and_8
; MIPS32: sync

define internal i32 @test_atomic_rmw_and_32_ignored(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.rmw.i32(i32 4, i32* %ptr, i32 %v, i32 6)
  ret i32 %v
}
; CHECK-LABEL: test_atomic_rmw_and_32_ignored
; Could just "lock and"
; CHECK: mov eax,DWORD PTR
; CHECK: and
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_and_32_ignored
; ARM32: dmb
; ARM32: ldrex
; ARM32: and
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_and_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: and
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

;; xor

define internal i32 @test_atomic_rmw_xor_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 5, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_xor_8
; CHECK: mov al,BYTE PTR
; CHECK: xor [[REG:[^a].]]
; CHECK: lock cmpxchg BYTE PTR [e{{[^a].}}],[[REG]]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_xor_8
; ARM32: dmb
; ARM32: ldrexb
; ARM32: eor
; ARM32: strexb
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xor_8
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: xor
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_rmw_xor_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 5, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_xor_16
; CHECK: mov ax,WORD PTR
; CHECK: xor
; CHECK: lock cmpxchg WORD PTR [e{{[^a].}}]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_xor_16
; ARM32: dmb
; ARM32: ldrexh
; ARM32: eor
; ARM32: strexh
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xor_16
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: xor
; MIPS32: and
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_rmw_xor_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 5, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_xor_32
; CHECK: mov eax,DWORD PTR
; CHECK: xor
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_xor_32
; ARM32: dmb
; ARM32: ldrex
; ARM32: eor
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xor_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: xor
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_rmw_xor_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 5, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_xor_64
; CHECK: push ebx
; CHECK: mov eax,DWORD PTR [{{.*}}]
; CHECK: mov edx,DWORD PTR [{{.*}}+0x4]
; CHECK: mov ebx,eax
; CHECK: or ebx,{{.*e.[^x]}}
; CHECK: mov ecx,edx
; CHECK: or ecx,{{.*e.[^x]}}
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_xor_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: eor
; ARM32: eor
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, [r{{[0-9]+}}]
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xor_64
; MIPS32: sync
; MIPS32: jal	__sync_fetch_and_xor_8
; MIPS32: sync

define internal i32 @test_atomic_rmw_xor_32_ignored(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.rmw.i32(i32 5, i32* %ptr, i32 %v, i32 6)
  ret i32 %v
}
; CHECK-LABEL: test_atomic_rmw_xor_32_ignored
; CHECK: mov eax,DWORD PTR
; CHECK: xor
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}]
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_xor_32_ignored
; ARM32: dmb
; ARM32: ldrex
; ARM32: eor
; ARM32: strex
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xor_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: xor
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

;; exchange

define internal i32 @test_atomic_rmw_xchg_8(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i8
  %ptr = inttoptr i32 %iptr to i8*
  %a = call i8 @llvm.nacl.atomic.rmw.i8(i32 6, i8* %ptr, i8 %trunc, i32 6)
  %a_ext = zext i8 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_xchg_8
; CHECK: xchg BYTE PTR {{.*}},[[REG:.*]]
; ARM32-LABEL: test_atomic_rmw_xchg_8
; ARM32: dmb
; ARM32: ldrexb
; ARM32: strexb
; ARM32: cmp
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xchg_8
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_rmw_xchg_16(i32 %iptr, i32 %v) {
entry:
  %trunc = trunc i32 %v to i16
  %ptr = inttoptr i32 %iptr to i16*
  %a = call i16 @llvm.nacl.atomic.rmw.i16(i32 6, i16* %ptr, i16 %trunc, i32 6)
  %a_ext = zext i16 %a to i32
  ret i32 %a_ext
}
; CHECK-LABEL: test_atomic_rmw_xchg_16
; CHECK: xchg WORD PTR {{.*}},[[REG:.*]]
; ARM32-LABEL: test_atomic_rmw_xchg_16
; ARM32: dmb
; ARM32: ldrexh
; ARM32: strexh
; ARM32: cmp
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xchg_16
; MIPS32: sync
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: sllv
; MIPS32: ll
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: and
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_rmw_xchg_32(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %a = call i32 @llvm.nacl.atomic.rmw.i32(i32 6, i32* %ptr, i32 %v, i32 6)
  ret i32 %a
}
; CHECK-LABEL: test_atomic_rmw_xchg_32
; CHECK: xchg DWORD PTR {{.*}},[[REG:.*]]
; ARM32-LABEL: test_atomic_rmw_xchg_32
; ARM32: dmb
; ARM32: ldrex
; ARM32: strex
; ARM32: cmp
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xchg_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: move
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_rmw_xchg_64(i32 %iptr, i64 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %a = call i64 @llvm.nacl.atomic.rmw.i64(i32 6, i64* %ptr, i64 %v, i32 6)
  ret i64 %a
}
; CHECK-LABEL: test_atomic_rmw_xchg_64
; CHECK: push ebx
; CHECK-DAG: mov edx
; CHECK-DAG: mov eax
; CHECK-DAG: mov ecx
; CHECK-DAG: mov ebx
; CHECK: lock cmpxchg8b QWORD PTR [{{e.[^x]}}
; CHECK: jne
; ARM32-LABEL: test_atomic_rmw_xchg_64
; ARM32: dmb
; ARM32: ldrexd r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[PTR:r[0-9]+]]{{[]]}}
; ARM32: strexd r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[PTR]]{{[]]}}
; ARM32: cmp
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xchg_64
; MIPS32: sync
; MIPS32: jal	__sync_lock_test_and_set_8
; MIPS32: sync

define internal i32 @test_atomic_rmw_xchg_32_ignored(i32 %iptr, i32 %v) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.rmw.i32(i32 6, i32* %ptr, i32 %v, i32 6)
  ret i32 %v
}
; In this case, ignoring the return value doesn't help. The xchg is
; used to do an atomic store.
; CHECK-LABEL: test_atomic_rmw_xchg_32_ignored
; CHECK: xchg DWORD PTR {{.*}},[[REG:.*]]
; ARM32-LABEL: test_atomic_rmw_xchg_32_ignored
; ARM32: dmb
; ARM32: ldrex
; ARM32: strex
; ARM32: cmp
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_rmw_xchg_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: move
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

;;;; Cmpxchg

define internal i32 @test_atomic_cmpxchg_8(i32 %iptr, i32 %expected,
                                           i32 %desired) {
entry:
  %trunc_exp = trunc i32 %expected to i8
  %trunc_des = trunc i32 %desired to i8
  %ptr = inttoptr i32 %iptr to i8*
  %old = call i8 @llvm.nacl.atomic.cmpxchg.i8(i8* %ptr, i8 %trunc_exp,
                                              i8 %trunc_des, i32 6, i32 6)
  %old_ext = zext i8 %old to i32
  ret i32 %old_ext
}
; CHECK-LABEL: test_atomic_cmpxchg_8
; CHECK: mov eax,{{.*}}
; Need to check that eax isn't used as the address register or the desired.
; since it is already used as the *expected* register.
; CHECK: lock cmpxchg BYTE PTR [e{{[^a].}}],{{[^a]}}l
; ARM32-LABEL: test_atomic_cmpxchg_8
; ARM32: dmb
; ARM32: ldrexb [[V:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: lsl [[VV:r[0-9]+]], [[V]], #24
; ARM32: cmp [[VV]], {{r[0-9]+}}, lsl #24
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexbeq [[SUCCESS]], {{r[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_8
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, $zero, 255
; MIPS32: sllv
; MIPS32: nor
; MIPS32: andi	{{.*}}, {{.*}}, 255
; MIPS32: sllv
; MIPS32: andi	{{.*}}, {{.*}}, 255
; MIPS32: sllv
; MIPS32: sync
; MIPS32: ll
; MIPS32: and
; MIPS32: bne
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	$zero, {{.*}}, {{.*}}
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 24
; MIPS32: sra	{{.*}}, {{.*}}, 24
; MIPS32: sync

define internal i32 @test_atomic_cmpxchg_16(i32 %iptr, i32 %expected,
                                            i32 %desired) {
entry:
  %trunc_exp = trunc i32 %expected to i16
  %trunc_des = trunc i32 %desired to i16
  %ptr = inttoptr i32 %iptr to i16*
  %old = call i16 @llvm.nacl.atomic.cmpxchg.i16(i16* %ptr, i16 %trunc_exp,
                                               i16 %trunc_des, i32 6, i32 6)
  %old_ext = zext i16 %old to i32
  ret i32 %old_ext
}
; CHECK-LABEL: test_atomic_cmpxchg_16
; CHECK: mov {{ax|eax}},{{.*}}
; CHECK: lock cmpxchg WORD PTR [e{{[^a].}}],{{[^a]}}x
; ARM32-LABEL: test_atomic_cmpxchg_16
; ARM32: dmb
; ARM32: ldrexh [[V:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: lsl [[VV:r[0-9]+]], [[V]], #16
; ARM32: cmp [[VV]], {{r[0-9]+}}, lsl #16
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexheq [[SUCCESS]], {{r[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_16
; MIPS32: addiu	{{.*}}, $zero, -4
; MIPS32: and
; MIPS32: andi	{{.*}}, {{.*}}, 3
; MIPS32: sll	{{.*}}, {{.*}}, 3
; MIPS32: ori	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: nor
; MIPS32: andi	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: andi	{{.*}}, {{.*}}, 65535
; MIPS32: sllv
; MIPS32: sync
; MIPS32: ll
; MIPS32: and
; MIPS32: bne
; MIPS32: and
; MIPS32: or
; MIPS32: sc
; MIPS32: beq	$zero, {{.*}}, {{.*}}
; MIPS32: srlv
; MIPS32: sll	{{.*}}, {{.*}}, 16
; MIPS32: sra	{{.*}}, {{.*}}, 16
; MIPS32: sync

define internal i32 @test_atomic_cmpxchg_32(i32 %iptr, i32 %expected,
                                            i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %old = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                               i32 %desired, i32 6, i32 6)
  ret i32 %old
}
; CHECK-LABEL: test_atomic_cmpxchg_32
; CHECK: mov eax,{{.*}}
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}],e{{[^a]}}
; ARM32-LABEL: test_atomic_cmpxchg_32
; ARM32: dmb
; ARM32: ldrex [[V:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexeq [[SUCCESS]], {{r[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_32
; MIPS32: sync
; MIPS32: ll
; MIPS32: bne
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_cmpxchg_64(i32 %iptr, i64 %expected,
                                            i64 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 %expected,
                                               i64 %desired, i32 6, i32 6)
  ret i64 %old
}
; CHECK-LABEL: test_atomic_cmpxchg_64
; CHECK: push ebx
; CHECK-DAG: mov edx
; CHECK-DAG: mov eax
; CHECK-DAG: mov ecx
; CHECK-DAG: mov ebx
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}+0x0]
; edx and eax are already the return registers, so they don't actually
; need to be reshuffled via movs. The next test stores the result
; somewhere, so in that case they do need to be mov'ed.
; ARM32-LABEL: test_atomic_cmpxchg_64
; ARM32: dmb
; ARM32: ldrexd [[V0:r[0-9]+]], [[V1:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V0]], {{r[0-9]+}}
; ARM32: cmpeq [[V1]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexdeq [[SUCCESS]], r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_64
; MIPS32: sync
; MIPS32: jal	__sync_val_compare_and_swap_8
; MIPS32: sync


define internal i64 @test_atomic_cmpxchg_64_undef(i32 %iptr, i64 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 undef,
                                               i64 %desired, i32 6, i32 6)
  ret i64 %old
}
; CHECK-LABEL: test_atomic_cmpxchg_64_undef
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}+0x0]
; ARM32-LABEL: test_atomic_cmpxchg_64_undef
; ARM32: mov r{{[0-9]+}}, #0
; ARM32: mov r{{[0-9]+}}, #0
; ARM32: dmb
; ARM32: ldrexd [[V0:r[0-9]+]], [[V1:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V0]], {{r[0-9]+}}
; ARM32: cmpeq [[V1]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexdeq [[SUCCESS]], r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_64_undef
; MIPS32: sync
; MIPS32: jal	__sync_val_compare_and_swap_8
; MIPS32: sync

; Test a case where %old really does need to be copied out of edx:eax.
define internal void @test_atomic_cmpxchg_64_store(
    i32 %ret_iptr, i32 %iptr, i64 %expected, i64 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 %expected,
                                                i64 %desired, i32 6, i32 6)
  %__6 = inttoptr i32 %ret_iptr to i64*
  store i64 %old, i64* %__6, align 1
  ret void
}
; CHECK-LABEL: test_atomic_cmpxchg_64_store
; CHECK: push ebx
; CHECK-DAG: mov edx
; CHECK-DAG: mov eax
; CHECK-DAG: mov ecx
; CHECK-DAG: mov ebx
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}
; CHECK-DAG: mov {{.*}},edx
; CHECK-DAG: mov {{.*}},eax
; ARM32-LABEL: test_atomic_cmpxchg_64_store
; ARM32: dmb
; ARM32: ldrexd [[V0:r[0-9]+]], [[V1:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V0]], {{r[0-9]+}}
; ARM32: cmpeq [[V1]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexdeq [[SUCCESS]], r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; ARM32: str
; ARM32: str
; MIPS32-LABEL: test_atomic_cmpxchg_64_store
; MIPS32: sync
; MIPS32: jal	__sync_val_compare_and_swap_8
; MIPS32: sync


; Test with some more register pressure. When we have an alloca, ebp is
; used to manage the stack frame, so it cannot be used as a register either.
define internal i64 @test_atomic_cmpxchg_64_alloca(i32 %iptr, i64 %expected,
                                                   i64 %desired) {
entry:
  br label %eblock  ; Disable alloca optimization
eblock:
  %alloca_ptr = alloca i8, i32 16, align 16
  %ptr = inttoptr i32 %iptr to i64*
  %old = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 %expected,
                                                i64 %desired, i32 6, i32 6)
  store i8 0, i8* %alloca_ptr, align 1
  store i8 1, i8* %alloca_ptr, align 1
  store i8 2, i8* %alloca_ptr, align 1
  store i8 3, i8* %alloca_ptr, align 1
  %__6 = ptrtoint i8* %alloca_ptr to i32
  call void @use_ptr(i32 %__6)
  ret i64 %old
}
; CHECK-LABEL: test_atomic_cmpxchg_64_alloca
; CHECK: push ebx
; CHECK-DAG: mov edx
; CHECK-DAG: mov eax
; CHECK-DAG: mov ecx
; CHECK-DAG: mov ebx
; Ptr cannot be eax, ebx, ecx, or edx (used up for the expected and desired).
; It also cannot be ebp since we use that for alloca. Also make sure it's
; not esp, since that's the stack pointer and mucking with it will break
; the later use_ptr function call.
; That pretty much leaves esi, or edi as the only viable registers.
; CHECK: lock cmpxchg8b QWORD PTR [e{{[ds]}}i]
; CHECK: call {{.*}} R_{{.*}} use_ptr
; ARM32-LABEL: test_atomic_cmpxchg_64_alloca
; ARM32: dmb
; ARM32: ldrexd [[V0:r[0-9]+]], [[V1:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V0]], {{r[0-9]+}}
; ARM32: cmpeq [[V1]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexdeq [[SUCCESS]], r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[A]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_64_alloca
; MIPS32: sync
; MIPS32: jal	__sync_val_compare_and_swap_8
; MIPS32: sync

define internal i32 @test_atomic_cmpxchg_32_ignored(i32 %iptr, i32 %expected,
                                                    i32 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i32*
  %ignored = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %ptr, i32 %expected,
                                                    i32 %desired, i32 6, i32 6)
  ret i32 0
}
; CHECK-LABEL: test_atomic_cmpxchg_32_ignored
; CHECK: mov eax,{{.*}}
; CHECK: lock cmpxchg DWORD PTR [e{{[^a].}}]
; ARM32-LABEL: test_atomic_cmpxchg_32_ignored
; ARM32: dmb
; ARM32: ldrex [[V:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexeq [[SUCCESS]]
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_32_ignored
; MIPS32: sync
; MIPS32: ll
; MIPS32: bne
; MIPS32: sc
; MIPS32: beq	{{.*}}, $zero, {{.*}}
; MIPS32: sync

define internal i64 @test_atomic_cmpxchg_64_ignored(i32 %iptr, i64 %expected,
                                                    i64 %desired) {
entry:
  %ptr = inttoptr i32 %iptr to i64*
  %ignored = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %ptr, i64 %expected,
                                                    i64 %desired, i32 6, i32 6)
  ret i64 0
}
; CHECK-LABEL: test_atomic_cmpxchg_64_ignored
; CHECK: push ebx
; CHECK-DAG: mov edx
; CHECK-DAG: mov eax
; CHECK-DAG: mov ecx
; CHECK-DAG: mov ebx
; CHECK: lock cmpxchg8b QWORD PTR [e{{.[^x]}}+0x0]
; ARM32-LABEL: test_atomic_cmpxchg_64_ignored
; ARM32: dmb
; ARM32: ldrexd [[V0:r[0-9]+]], [[V1:r[0-9]+]], {{[[]}}[[A:r[0-9]+]]{{[]]}}
; ARM32: cmp [[V0]], {{r[0-9]+}}
; ARM32: cmpeq [[V1]], {{r[0-9]+}}
; ARM32: movne [[SUCCESS:r[0-9]+]],
; ARM32: strexdeq [[SUCCESS]], r{{[0-9]+}}, r{{[0-9]+}}, {{[[]}}[[PTR]]{{[]]}}
; ARM32: cmp [[SUCCESS]], #0
; ARM32: bne
; ARM32: dmb
; MIPS32-LABEL: test_atomic_cmpxchg_64_ignored
; MIPS32: sync
; MIPS32: jal	__sync_val_compare_and_swap_8
; MIPS32: sync

;;;; Fence and is-lock-free.

define internal void @test_atomic_fence() {
entry:
  call void @llvm.nacl.atomic.fence(i32 6)
  ret void
}
; CHECK-LABEL: test_atomic_fence
; CHECK: mfence
; ARM32-LABEL: test_atomic_fence
; ARM32: dmb sy
; MIPS32-LABEL: test_atomic_fence
; MIPS32: sync

define internal void @test_atomic_fence_all() {
entry:
  call void @llvm.nacl.atomic.fence.all()
  ret void
}
; CHECK-LABEL: test_atomic_fence_all
; CHECK: mfence
; ARM32-LABEL: test_atomic_fence_all
; ARM32: dmb sy
; MIPS32-LABEL: test_atomic_fence_all
; MIPS32: sync

define internal i32 @test_atomic_is_lock_free(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i1 @llvm.nacl.atomic.is.lock.free(i32 4, i8* %ptr)
  %r = zext i1 %i to i32
  ret i32 %r
}
; CHECK-LABEL: test_atomic_is_lock_free
; CHECK: mov {{.*}},0x1
; ARM32-LABEL: test_atomic_is_lock_free
; ARM32: mov {{.*}}, #1
; MIPS32-LABEL: test_atomic_is_lock_free
; MIPS32: addiu {{.*}}, $zero, 1

define internal i32 @test_not_lock_free(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i1 @llvm.nacl.atomic.is.lock.free(i32 7, i8* %ptr)
  %r = zext i1 %i to i32
  ret i32 %r
}
; CHECK-LABEL: test_not_lock_free
; CHECK: mov {{.*}},0x0
; ARM32-LABEL: test_not_lock_free
; ARM32: mov {{.*}}, #0
; MIPS32-LABEL: test_not_lock_free
; MIPS32: addiu {{.*}}, $zero, 0

define internal i32 @test_atomic_is_lock_free_ignored(i32 %iptr) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %ignored = call i1 @llvm.nacl.atomic.is.lock.free(i32 4, i8* %ptr)
  ret i32 0
}
; CHECK-LABEL: test_atomic_is_lock_free_ignored
; CHECK: mov {{.*}},0x0
; This can get optimized out, because it's side-effect-free.
; O2-LABEL: test_atomic_is_lock_free_ignored
; O2-NOT: mov {{.*}}, 1
; O2: mov {{.*}},0x0
; ARM32O2-LABEL: test_atomic_is_lock_free_ignored
; ARM32O2-NOT: mov {{.*}}, #1
; ARM32O2: mov {{.*}}, #0
; MIPS32O2-LABEL: test_atomic_is_lock_free
; MIPS32O2-NOT: addiu {{.*}}, $zero, 1
; MIPS32O2: addiu {{.*}}, $zero, 0

; TODO(jvoung): at some point we can take advantage of the
; fact that nacl.atomic.is.lock.free will resolve to a constant
; (which adds DCE opportunities). Once we optimize, the test expectations
; for this case should change.
define internal i32 @test_atomic_is_lock_free_can_dce(i32 %iptr, i32 %x,
                                                      i32 %y) {
entry:
  %ptr = inttoptr i32 %iptr to i8*
  %i = call i1 @llvm.nacl.atomic.is.lock.free(i32 4, i8* %ptr)
  %i_ext = zext i1 %i to i32
  %cmp = icmp eq i32 %i_ext, 1
  br i1 %cmp, label %lock_free, label %not_lock_free
lock_free:
  ret i32 %i_ext

not_lock_free:
  %z = add i32 %x, %y
  ret i32 %z
}
; CHECK-LABEL: test_atomic_is_lock_free_can_dce
; CHECK: mov {{.*}},0x1
; CHECK: ret
; CHECK: add
; CHECK: ret

; Test the liveness / register allocation properties of the xadd instruction.
; Make sure we model that the Src register is modified and therefore it can't
; share a register with an overlapping live range, even if the result of the
; xadd instruction is unused.
define internal void @test_xadd_regalloc() {
entry:
  br label %body
body:
  %i = phi i32 [ 1, %entry ], [ %i_plus_1, %body ]
  %g = bitcast [4 x i8]* @SzGlobal32 to i32*
  %unused = call i32 @llvm.nacl.atomic.rmw.i32(i32 1, i32* %g, i32 %i, i32 6)
  %i_plus_1 = add i32 %i, 1
  %cmp = icmp eq i32 %i_plus_1, 1001
  br i1 %cmp, label %done, label %body
done:
  ret void
}
; O2-LABEL: test_xadd_regalloc
;;; Some register will be used in the xadd instruction.
; O2: lock xadd DWORD PTR {{.*}},[[REG:e..]]
;;; Make sure that register isn't used again, e.g. as the induction variable.
; O2-NOT: ,[[REG]]
; O2: ret

; Do the same test for the xchg instruction instead of xadd.
define internal void @test_xchg_regalloc() {
entry:
  br label %body
body:
  %i = phi i32 [ 1, %entry ], [ %i_plus_1, %body ]
  %g = bitcast [4 x i8]* @SzGlobal32 to i32*
  %unused = call i32 @llvm.nacl.atomic.rmw.i32(i32 6, i32* %g, i32 %i, i32 6)
  %i_plus_1 = add i32 %i, 1
  %cmp = icmp eq i32 %i_plus_1, 1001
  br i1 %cmp, label %done, label %body
done:
  ret void
}
; O2-LABEL: test_xchg_regalloc
;;; Some register will be used in the xchg instruction.
; O2: xchg DWORD PTR {{.*}},[[REG:e..]]
;;; Make sure that register isn't used again, e.g. as the induction variable.
; O2-NOT: ,[[REG]]
; O2: ret

; Same test for cmpxchg.
define internal void @test_cmpxchg_regalloc() {
entry:
  br label %body
body:
  %i = phi i32 [ 1, %entry ], [ %i_plus_1, %body ]
  %g = bitcast [4 x i8]* @SzGlobal32 to i32*
  %unused = call i32 @llvm.nacl.atomic.cmpxchg.i32(i32* %g, i32 %i, i32 %i, i32 6, i32 6)
  %i_plus_1 = add i32 %i, 1
  %cmp = icmp eq i32 %i_plus_1, 1001
  br i1 %cmp, label %done, label %body
done:
  ret void
}
; O2-LABEL: test_cmpxchg_regalloc
;;; eax and some other register will be used in the cmpxchg instruction.
; O2: lock cmpxchg DWORD PTR {{.*}},[[REG:e..]]
;;; Make sure eax isn't used again, e.g. as the induction variable.
; O2-NOT: ,eax
; O2: ret

; Same test for cmpxchg8b.
define internal void @test_cmpxchg8b_regalloc() {
entry:
  br label %body
body:
  %i = phi i32 [ 1, %entry ], [ %i_plus_1, %body ]
  %g = bitcast [8 x i8]* @SzGlobal64 to i64*
  %i_64 = zext i32 %i to i64
  %unused = call i64 @llvm.nacl.atomic.cmpxchg.i64(i64* %g, i64 %i_64, i64 %i_64, i32 6, i32 6)
  %i_plus_1 = add i32 %i, 1
  %cmp = icmp eq i32 %i_plus_1, 1001
  br i1 %cmp, label %done, label %body
done:
  ret void
}
; O2-LABEL: test_cmpxchg8b_regalloc
;;; eax and some other register will be used in the cmpxchg instruction.
; O2: lock cmpxchg8b QWORD PTR
;;; Make sure eax/ecx/edx/ebx aren't used again, e.g. as the induction variable.
; O2-NOT: ,{{eax|ecx|edx|ebx}}
; O2: pop ebx
