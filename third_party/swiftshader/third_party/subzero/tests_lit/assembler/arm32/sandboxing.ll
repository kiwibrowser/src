; Tests basics and corner cases of arm32 sandboxing, using -Om1 in the hope that
; the output will remain stable.  When packing bundles, we try to limit to a few
; instructions with well known sizes and minimal use of registers and stack
; slots in the lowering sequence.

; REQUIRES: allow_dump, target_ARM32
; RUN: %p2i -i %s --sandbox --filetype=asm --target=arm32 --assemble \
; RUN:   --disassemble --args -Om1 -allow-externally-defined-symbols \
; RUN:   -ffunction-sections -reg-use r0,r1,r3 | FileCheck %s

declare void @call_target()
declare void @call_target1(i32 %arg0)
declare void @call_target2(i32 %arg0, i32 %arg1)
declare void @call_target3(i32 %arg0, i32 %arg1, i32 %arg2)
@global_short = internal global [2 x i8] zeroinitializer

; A direct call sequence uses the right mask and register-call sequence.
define internal void @test_direct_call() {
entry:
  call void @call_target()
  ; bundle aigned.
  ret void
}

; CHECK-LABEL:<test_direct_call>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.*}} bl {{.*}} call_target

; Same as above, but force bundle padding by adding three (branch) instruction
; before the tested call.
define internal void @test_direct_call_with_padding_1() {
entry:
  call void @call_target()
  ; bundle aigned.

  br label %next1 ; add 1 inst.
next1:
  br label %next2 ; add 1 inst.
next2:
  call void @call_target()
  ret void
}
; CHECK-LABEL:<test_direct_call_with_padding_1>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: b
; CHECK-NEXT: nop
; CHECK-NEXT: bl {{.*}} call_target
; CHECK-NEXT: {{[0-9a-f]*}}0:

; Same as above, but force bundle padding by adding two (branch) instruction
; before the tested call.
define internal void @test_direct_call_with_padding_2() {
entry:
  call void @call_target()
  ; bundle aigned.

  br label %next1 ; add 1 inst.
next1:
  call void @call_target()
  ret void
}

; CHECK-LABEL:<test_direct_call_with_padding_2>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: nop
; CHECK-NEXT: nop
; CHECK-NEXT: bl {{.*}} call_target
; CHECK-NEXT: {{[0-9a-f]*}}0:

; Same as above, but force bundle padding by adding single (branch) instruction
; before the tested call.
define internal void @test_direct_call_with_padding_3() {
entry:
  call void @call_target()
  ; bundle aigned.

  call void @call_target()
  ret void
}

; CHECK-LABEL:<test_direct_call_with_padding_3>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: nop
; CHECK-NEXT: nop
; CHECK-NEXT: nop
; CHECK-NEXT: bl {{.*}} call_target
; CHECK-NEXT: {{[0-9a-f]*}}0:

; An indirect call sequence uses the right mask and register-call sequence.
define internal void @test_indirect_call(i32 %target) {
entry:
  %__1 = inttoptr i32 %target to void ()*
  call void @call_target();
  ; bundle aigned.

  br label %next ; add 1 inst.
next:
  call void %__1() ; requires 3 insts.
  ret void
}

; CHECK-LABEL:<test_indirect_call>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: ldr
; CHECK-NEXT: bic [[REG:r[0-3]]], [[REG]], {{.*}} 0xc000000f
; CHECK-NEXT: blx [[REG]]
; CHECK-NEXT: {{[0-9]+}}0:

; An indirect call sequence uses the right mask and register-call sequence.
; Forces bundling before the tested call.
define internal void @test_indirect_call_with_padding_1(i32 %target) {
entry:
  %__1 = inttoptr i32 %target to void ()*
  call void @call_target();
  ; bundle aigned.
  call void %__1() ; requires 3 insts.
  ret void
}

; CHECK-LABEL: <test_indirect_call_with_padding_1>:
;              Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: ldr
; CHECK-NEXT: nop
; CHECK-NEXT: bic [[REG:r[0-3]]], [[REG]], {{.*}} 0xc000000f
; CHECK-NEXT: blx [[REG]]
; CHECK-NEXT: {{[0-9]+}}0:

; An indirect call sequence uses the right mask and register-call sequence.
; Forces bundling by adding three (branch) instructions befor the tested call.
define internal void @test_indirect_call_with_padding_2(i32 %target) {
entry:
  %__1 = inttoptr i32 %target to void ()*
  call void @call_target();
  ; bundle aigned.

  br label %next1 ; add 1 inst.
next1:
  br label %next2 ; add 1 inst.
next2:
  br label %next3 ; add 1 inst.
next3:
  call void %__1() ; requires 3 insts.
  ret void
}

; CHECK-LABEL: <test_indirect_call_with_padding_2>:
;              Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: b
; CHECK-NEXT: b
; CHECK-NEXT: ldr
; CHECK-NEXT: nop
; CHECK-NEXT: nop
; CHECK-NEXT: bic [[REG:r[0-3]]], [[REG]], {{.*}} 0xc000000f
; CHECK-NEXT: blx [[REG]]
; CHECK-NEXT: {{[0-9]+}}0:

; An indirect call sequence uses the right mask and register-call sequence.
; Forces bundling by adding two (branch) instructions befor the tested call.
define internal void @test_indirect_call_with_padding_3(i32 %target) {
entry:
  %__1 = inttoptr i32 %target to void ()*
  call void @call_target();
  ; bundle aigned.

  br label %next1 ; add 1 inst
next1:
  br label %next2 ; add 1 inst
next2:
  call void %__1() ; requires 3 insts.
  ret void
}
; CHECK-LABEL: <test_indirect_call_with_padding_3>:
;              Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: b
; CHECK-NEXT: ldr
; CHECK-NEXT: nop
; CHECK-NEXT: nop
; CHECK-NEXT: nop
; CHECK-NEXT: bic [[REG:r[0-3]]], [[REG]], {{.*}} 0xc000000f
; CHECK-NEXT: blx [[REG]]
; CHECK-NEXT: {{[0-9]+}}0:

; A return sequences uses the right pop / mask / jmp sequence.
define internal void @test_ret() {
entry:
  call void @call_target()
  ; Bundle boundary.
  br label %next ; add 1 inst.
next:
  ret void
}
; CHECK-LABEL:<test_ret>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: add sp, sp
; CHECK-NEXT: bic sp, sp, {{.+}} ; 0xc0000000
; CHECK-NEXT: pop {lr}
; CHECK-NEXT: {{[0-9a-f]*}}0: {{.+}} bic lr, lr, {{.+}} ; 0xc000000f
; CHECK-NEXT: bx lr

; A return sequence with padding for bundle lock.
define internal void @test_ret_with_padding() {
  call void @call_target()
  ; Bundle boundary.
  ret void
}

; CHECK-LABEL:<test_ret_with_padding>:
;             Search for bundle alignment of first call.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: add sp, sp
; CHECK-NEXT: bic sp, sp, {{.+}} ; 0xc0000000
; CHECK-NEXT: pop {lr}
; CHECK-NEXT: nop
; CHECK-NEXT: {{[0-9a-f]*}}0: {{.+}} bic lr, lr, {{.+}} ; 0xc000000f
; CHECK-NEXT: bx lr

; Store without bundle padding.
define internal void @test_store() {
entry:
  call void @call_target()
  ; Bundle boundary
  store i16 1, i16* undef, align 1   ; 3 insts + bic.
  ret void
}

; CHECK-LABEL: test_store
;             Search for call at end of bundle.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: mov [[REG:r[0-9]]], #0
; CHECK-NEXT: mov
; CHECK-NEXT: bic [[REG]], [[REG]], {{.+}} ; 0xc0000000
; CHECK-NEXT: strh r{{.+}}[[REG]]

; Store with bundle padding. Force padding by adding a single branch
; instruction.
define internal void @test_store_with_padding() {
entry:
  call void @call_target()
  ; bundle boundary
  br label %next ; add 1 inst.
next:
  store i16 0, i16* undef, align 1   ; 3 insts
  ret void
}
; CHECK-LABEL: test_store_with_padding
;             Search for call at end of bundle.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: mov [[REG:r[0-9]]], #0
; CHECK-NEXT: mov
; CHECK-NEXT: nop
; CHECK-NEXT: bic [[REG]], [[REG]], {{.+}} ; 0xc0000000
; CHECK-NEXT: strh r{{.+}}[[REG]]


; Store without bundle padding.
define internal i32 @test_load() {
entry:
  call void @call_target()
  ; Bundle boundary
  %v = load i32, i32* undef, align 1 ; 4 insts, bundling middle 2.
  ret i32 %v
}

; CHECK-LABEL: test_load
;             Search for call at end of bundle.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: mov [[REG:r[0-9]]], #0
; CHECK-NEXT: bic [[REG]], [[REG]], {{.+}} ; 0xc0000000
; CHECK-NEXT: ldr r{{.+}}[[REG]]

; Store with bundle padding.
define internal i32 @test_load_with_padding() {
entry:
  call void @call_target()
  ; Bundle boundary
  br label %next1 ; add 1 inst.
next1:
  br label %next2 ; add 1 inst.
next2:
  %v = load i32, i32* undef, align 1 ; 4 insts, bundling middle 2.
  ret i32 %v
}

; CHECK-LABEL: test_load_with_padding
;             Search for call at end of bundle.
; CHECK:      {{[0-9a-f]*}}c: {{.+}} bl
; CHECK-NEXT: b
; CHECK-NEXT: b
; CHECK-NEXT: mov [[REG:r[0-9]]], #0
; CHECK-NEXT: nop
; CHECK-NEXT: bic [[REG]], [[REG]], {{.+}} ; 0xc0000000
; CHECK-NEXT: ldr r{{.+}}[[REG]]
