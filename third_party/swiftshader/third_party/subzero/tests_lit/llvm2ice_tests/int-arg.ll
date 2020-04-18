; This file checks that Subzero generates code in accordance with the
; calling convention for integers.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; TODO: Switch to --filetype=obj when possible.
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; For x86-32, integer arguments use the stack.
; For ARM32, integer arguments can be r0-r3. i64 arguments occupy two
; adjacent 32-bit registers, and require the first to be an even register.


; i32

define internal i32 @test_returning32_arg0(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
entry:
  ret i32 %arg0
}
; CHECK-LABEL: test_returning32_arg0
; CHECK-NEXT: mov eax,{{.*}} [esp+0x4]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_arg0
; ARM32-NEXT: bx lr
; MIPS32-LABEL: test_returning32_arg0
; MIPS32: move v0,a0
; MIPS32-NEXT: jr       ra

define internal i32 @test_returning32_arg1(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
entry:
  ret i32 %arg1
}
; CHECK-LABEL: test_returning32_arg1
; CHECK-NEXT: mov eax,{{.*}} [esp+0x8]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_arg1
; ARM32-NEXT: mov r0, r1
; ARM32-NEXT: bx lr
; MIPS32-LABEL: test_returning32_arg1
; MIPS32: move v0,a1
; MIPS32-NEXT: jr       ra

define internal i32 @test_returning32_arg2(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
entry:
  ret i32 %arg2
}
; CHECK-LABEL: test_returning32_arg2
; CHECK-NEXT: mov eax,{{.*}} [esp+0xc]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_arg2
; ARM32-NEXT: mov r0, r2
; ARM32-NEXT: bx lr
; MIPS32-LABEL: test_returning32_arg2
; MIPS32: move v0,a2
; MIPS32-NEXT: jr       ra


define internal i32 @test_returning32_arg3(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
entry:
  ret i32 %arg3
}
; CHECK-LABEL: test_returning32_arg3
; CHECK-NEXT: mov eax,{{.*}} [esp+0x10]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_arg3
; ARM32-NEXT: mov r0, r3
; ARM32-NEXT: bx lr


define internal i32 @test_returning32_arg4(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
entry:
  ret i32 %arg4
}
; CHECK-LABEL: test_returning32_arg4
; CHECK-NEXT: mov eax,{{.*}} [esp+0x14]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_arg4
; ARM32-NEXT: ldr r0, [sp]
; ARM32-NEXT: bx lr


define internal i32 @test_returning32_arg5(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7) {
entry:
  ret i32 %arg5
}
; CHECK-LABEL: test_returning32_arg5
; CHECK-NEXT: mov eax,{{.*}} [esp+0x18]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_arg5
; ARM32-NEXT: ldr r0, [sp, #4]
; ARM32-NEXT: bx lr

; i64

define internal i64 @test_returning64_arg0(i64 %arg0, i64 %arg1, i64 %arg2, i64 %arg3) {
entry:
  ret i64 %arg0
}
; CHECK-LABEL: test_returning64_arg0
; CHECK-NEXT: mov {{.*}} [esp+0x4]
; CHECK-NEXT: mov {{.*}} [esp+0x8]
; CHECK: ret
; ARM32-LABEL: test_returning64_arg0
; ARM32-NEXT: bx lr
; MIPS32-LABEL: test_returning64_arg0
; MIPS32-NEXT: move v0,a0
; MIPS32-NEXT: move v1,a1


define internal i64 @test_returning64_arg1(i64 %arg0, i64 %arg1, i64 %arg2, i64 %arg3) {
entry:
  ret i64 %arg1
}
; CHECK-LABEL: test_returning64_arg1
; CHECK-NEXT: mov {{.*}} [esp+0xc]
; CHECK-NEXT: mov {{.*}} [esp+0x10]
; CHECK: ret
; ARM32-LABEL: test_returning64_arg1
; ARM32-NEXT: mov r0, r2
; ARM32-NEXT: mov r1, r3
; ARM32-NEXT: bx lr
; MIPS32-LABEL: test_returning64_arg1
; MIPS32-NEXT: move v0,a2
; MIPS32-NEXT: move v1,a3

define internal i64 @test_returning64_arg2(i64 %arg0, i64 %arg1, i64 %arg2, i64 %arg3) {
entry:
  ret i64 %arg2
}
; CHECK-LABEL: test_returning64_arg2
; CHECK-NEXT: mov {{.*}} [esp+0x14]
; CHECK-NEXT: mov {{.*}} [esp+0x18]
; CHECK: ret
; ARM32-LABEL: test_returning64_arg2
; This could have been a ldm sp, {r0, r1}, but we don't do the ldm optimization.
; ARM32-NEXT: ldr r0, [sp]
; ARM32-NEXT: ldr r1, [sp, #4]
; ARM32-NEXT: bx lr

define internal i64 @test_returning64_arg3(i64 %arg0, i64 %arg1, i64 %arg2, i64 %arg3) {
entry:
  ret i64 %arg3
}
; CHECK-LABEL: test_returning64_arg3
; CHECK-NEXT: mov {{.*}} [esp+0x1c]
; CHECK-NEXT: mov {{.*}} [esp+0x20]
; CHECK: ret
; ARM32-LABEL: test_returning64_arg3
; ARM32-NEXT: ldr r0, [sp, #8]
; ARM32-NEXT: ldr r1, [sp, #12]
; ARM32-NEXT: bx lr


; Test that on ARM, the i64 arguments start with an even register.

define internal i64 @test_returning64_even_arg1(i32 %arg0, i64 %arg1, i64 %arg2) {
entry:
  ret i64 %arg1
}
; Not padded out x86-32.
; CHECK-LABEL: test_returning64_even_arg1
; CHECK-NEXT: mov {{.*}} [esp+0x8]
; CHECK-NEXT: mov {{.*}} [esp+0xc]
; CHECK: ret
; ARM32-LABEL: test_returning64_even_arg1
; ARM32-NEXT: mov r0, r2
; ARM32-NEXT: mov r1, r3
; ARM32-NEXT: bx lr

define internal i64 @test_returning64_even_arg1b(i32 %arg0, i32 %arg0b, i64 %arg1, i64 %arg2) {
entry:
  ret i64 %arg1
}
; CHECK-LABEL: test_returning64_even_arg1b
; CHECK-NEXT: mov {{.*}} [esp+0xc]
; CHECK-NEXT: mov {{.*}} [esp+0x10]
; CHECK: ret
; ARM32-LABEL: test_returning64_even_arg1b
; ARM32-NEXT: mov r0, r2
; ARM32-NEXT: mov r1, r3
; ARM32-NEXT: bx lr

define internal i64 @test_returning64_even_arg2(i64 %arg0, i32 %arg1, i64 %arg2) {
entry:
  ret i64 %arg2
}
; Not padded out on x86-32.
; CHECK-LABEL: test_returning64_even_arg2
; CHECK-NEXT: mov {{.*}} [esp+0x10]
; CHECK-NEXT: mov {{.*}} [esp+0x14]
; CHECK: ret
; ARM32-LABEL: test_returning64_even_arg2
; ARM32-DAG: ldr r0, [sp]
; ARM32-DAG: ldr r1, [sp, #4]
; ARM32-NEXT: bx lr

define internal i64 @test_returning64_even_arg2b(i64 %arg0, i32 %arg1, i32 %arg1b, i64 %arg2) {
entry:
  ret i64 %arg2
}
; CHECK-LABEL: test_returning64_even_arg2b
; CHECK-NEXT: mov {{.*}} [esp+0x14]
; CHECK-NEXT: mov {{.*}} [esp+0x18]
; CHECK: ret
; ARM32-LABEL: test_returning64_even_arg2b
; ARM32-NEXT: ldr r0, [sp]
; ARM32-NEXT: ldr r1, [sp, #4]
; ARM32-NEXT: bx lr

define internal i32 @test_returning32_even_arg2(i64 %arg0, i32 %arg1, i32 %arg2) {
entry:
  ret i32 %arg2
}
; CHECK-LABEL: test_returning32_even_arg2
; CHECK-NEXT: mov {{.*}} [esp+0x10]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_even_arg2
; ARM32-NEXT: mov r0, r3
; ARM32-NEXT: bx lr

define internal i32 @test_returning32_even_arg2b(i32 %arg0, i32 %arg1, i32 %arg2, i64 %arg3) {
entry:
  ret i32 %arg2
}
; CHECK-LABEL: test_returning32_even_arg2b
; CHECK-NEXT: mov {{.*}} [esp+0xc]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_even_arg2b
; ARM32-NEXT: mov r0, r2
; ARM32-NEXT: bx lr

; The i64 won't fit in a pair of register, and consumes the last register so a
; following i32 can't use that free register.
define internal i32 @test_returning32_even_arg4(i32 %arg0, i32 %arg1, i32 %arg2, i64 %arg3, i32 %arg4) {
entry:
  ret i32 %arg4
}
; CHECK-LABEL: test_returning32_even_arg4
; CHECK-NEXT: mov {{.*}} [esp+0x18]
; CHECK-NEXT: ret
; ARM32-LABEL: test_returning32_even_arg4
; ARM32-NEXT: ldr r0, [sp, #8]
; ARM32-NEXT: bx lr

; Test interleaving float/double and integer (different register streams on ARM).
; TODO(jvoung): Test once the S/D/Q regs are modeled.

; Test that integers are passed correctly as arguments to a function.

declare void @IntArgs(i32, i32, i32, i32, i32, i32)

declare void @killRegisters()

define internal void @test_passing_integers(i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3, i32 %arg4, i32 %arg5, i32 %arg6) {
  call void @killRegisters()
  call void @IntArgs(i32 %arg6, i32 %arg5, i32 %arg4, i32 %arg3, i32 %arg2, i32 %arg1)
  ret void
}

; CHECK-LABEL: test_passing_integers
; CHECK-DAG: mov [[REG1:e.*]],DWORD PTR [esp+0x44]
; CHECK-DAG: mov [[REG2:e.*]],DWORD PTR [esp+0x48]
; CHECK-DAG: mov [[REG3:e.*]],DWORD PTR [esp+0x4c]
; CHECK-DAG: mov [[REG4:e.*]],DWORD PTR [esp+0x50]
; CHECK: mov DWORD PTR [esp]
; CHECK: mov DWORD PTR [esp+0x4]
; CHECK-DAG: mov DWORD PTR [esp+0x8],[[REG4]]
; CHECK-DAG: mov DWORD PTR [esp+0xc],[[REG3]]
; CHECK-DAG: mov DWORD PTR [esp+0x10],[[REG2]]
; CHECK-DAG: mov DWORD PTR [esp+0x14],[[REG1]]
; CHECK: call

; ARM32-LABEL: test_passing_integers
; ARM32-DAG: mov [[REG1:.*]], r1
; ARM32-DAG: mov [[REG2:.*]], r2
; ARM32-DAG: mov [[REG3:.*]], r3
; ARM32: str [[REG2]], [sp]
; ARM32: str [[REG1]], [sp, #4]
; ARM32-DAG: mov r0
; ARM32-DAG: mov r1
; ARM32-DAG: mov r2
; ARM32-DAG: mov r3, [[REG3]]
; ARM32: bl
