; This tries to be a comprehensive test of i64 operations, in
; particular the patterns for lowering i64 operations into constituent
; i32 operations on x86-32.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck --check-prefix=OPTM1 %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix ARM32-O2 %s
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix ARM32-OM1 %s

; TODO: Switch to --filetype=obj when possible.
; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 --check-prefix MIPS32-O2 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 --check-prefix MIPS32-OM1 %s

@__init_array_start = internal constant [0 x i8] zeroinitializer, align 4
@__fini_array_start = internal constant [0 x i8] zeroinitializer, align 4
@__tls_template_start = internal constant [0 x i8] zeroinitializer, align 8
@__tls_template_alignment = internal constant [4 x i8] c"\01\00\00\00", align 4

define internal i32 @ignore64BitArg(i64 %a, i32 %b, i64 %c) {
entry:
  ret i32 %b
}

; MIPS32-LABEL: ignore64BitArg
; MIPS32-O2: move v0,a2
; MIPS32-OM1: sw a2,[[MEM:.*]]
; MIPS32-OM1: lw v0,[[MEM]]

define internal i32 @pass64BitArg(i64 %a, i64 %b, i64 %c, i64 %d, i64 %e, i64 %f) {
entry:
  %call = call i32 @ignore64BitArgNoInline(i64 %a, i32 123, i64 %b)
  %call1 = call i32 @ignore64BitArgNoInline(i64 %c, i32 123, i64 %d)
  %call2 = call i32 @ignore64BitArgNoInline(i64 %e, i32 123, i64 %f)
  %add = add i32 %call1, %call
  %add3 = add i32 %add, %call2
  ret i32 %add3
}
; CHECK-LABEL: pass64BitArg
; CHECK:      sub     esp
; CHECK:      mov     DWORD PTR [esp+0x4]
; CHECK:      mov     DWORD PTR [esp]
; CHECK:      mov     DWORD PTR [esp+0x8],0x7b
; CHECK:      mov     DWORD PTR [esp+0x10]
; CHECK:      mov     DWORD PTR [esp+0xc]
; CHECK:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline
; CHECK:      mov     DWORD PTR [esp+0x4]
; CHECK:      mov     DWORD PTR [esp]
; CHECK:      mov     DWORD PTR [esp+0x8],0x7b
; CHECK:      mov     DWORD PTR [esp+0x10]
; CHECK:      mov     DWORD PTR [esp+0xc]
; CHECK:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline
; CHECK:      mov     DWORD PTR [esp+0x4]
; CHECK:      mov     DWORD PTR [esp]
; CHECK:      mov     DWORD PTR [esp+0x8],0x7b
; CHECK:      mov     DWORD PTR [esp+0x10]
; CHECK:      mov     DWORD PTR [esp+0xc]
; CHECK:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline
;
; OPTM1-LABEL: pass64BitArg
; OPTM1:      sub     esp
; OPTM1:      mov     DWORD PTR [esp+0x4]
; OPTM1:      mov     DWORD PTR [esp]
; OPTM1:      mov     DWORD PTR [esp+0x8],0x7b
; OPTM1:      mov     DWORD PTR [esp+0x10]
; OPTM1:      mov     DWORD PTR [esp+0xc]
; OPTM1:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline
; OPTM1:      mov     DWORD PTR [esp+0x4]
; OPTM1:      mov     DWORD PTR [esp]
; OPTM1:      mov     DWORD PTR [esp+0x8],0x7b
; OPTM1:      mov     DWORD PTR [esp+0x10]
; OPTM1:      mov     DWORD PTR [esp+0xc]
; OPTM1:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline
; OPTM1:      mov     DWORD PTR [esp+0x4]
; OPTM1:      mov     DWORD PTR [esp]
; OPTM1:      mov     DWORD PTR [esp+0x8],0x7b
; OPTM1:      mov     DWORD PTR [esp+0x10]
; OPTM1:      mov     DWORD PTR [esp+0xc]
; OPTM1:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline

; ARM32-LABEL: pass64BitArg
; ARM32:      str     {{.*}}, [sp]
; ARM32:      mov     r2, #123
; ARM32:      bl      {{.*}} ignore64BitArgNoInline
; ARM32:      str     {{.*}}, [sp]
; ARM32:      {{mov|ldr}} r0
; ARM32:      {{mov|ldr}} r1
; ARM32:      mov     r2, #123
; ARM32:      bl      {{.*}} ignore64BitArgNoInline
; ARM32:      str     {{.*}}, [sp]
; ARM32:      {{mov|ldr}} r0
; ARM32:      {{mov|ldr}} r1
; ARM32:      mov     r2, #123
; ARM32:      bl      {{.*}} ignore64BitArgNoInline

; MIPS32-LABEL: pass64BitArg
; MIPS32-O2: 	sw	a3,{{.*}}(sp)
; MIPS32-O2: 	sw	a2,{{.*}}(sp)
; MIPS32-O2: 	li	a2,123
; MIPS32-O2: 	jal	{{.*}}	ignore64BitArgNoInline
; MIPS32-O2: 	nop
; MIPS32-O2: 	move	s0,v0
; MIPS32-O2: 	sw	s3,{{.*}}(sp)
; MIPS32-O2: 	sw	s2,{{.*}}(sp)
; MIPS32-O2: 	lw	a0,{{.*}}(sp)
; MIPS32-O2: 	move	a1,s1
; MIPS32-O2: 	li	a2,123
; MIPS32-O2: 	jal	{{.*}}	ignore64BitArgNoInline
; MIPS32-O2: 	nop
; MIPS32-O2: 	move	s1,v0
; MIPS32-O2: 	sw	s7,{{.*}}(sp)
; MIPS32-O2: 	sw	s6,{{.*}}(sp)
; MIPS32-O2: 	move	a0,s4
; MIPS32-O2: 	move	a1,s5
; MIPS32-O2: 	li	a2,123
; MIPS32-O2: 	jal	{{.*}}	ignore64BitArgNoInline
; MIPS32-O2: 	nop


declare i32 @ignore64BitArgNoInline(i64, i32, i64)

define internal i32 @pass64BitConstArg(i64 %a, i64 %b) {
entry:
  %call = call i32 @ignore64BitArgNoInline(i64 %b, i32 123, i64 -2401053092306725256)
  ret i32 %call
}
; CHECK-LABEL: pass64BitConstArg
; CHECK:      sub     esp
; CHECK:      mov     DWORD PTR [esp+0x4]
; CHECK-NEXT: mov     DWORD PTR [esp]
; CHECK-NEXT: mov     DWORD PTR [esp+0x8],0x7b
; Bundle padding might be added (so not using -NEXT).
; CHECK:      mov     DWORD PTR [esp+0x10],0xdeadbeef
; CHECK-NEXT: mov     DWORD PTR [esp+0xc],0x12345678
; Bundle padding will push the call down.
; CHECK-NOT:  mov
; CHECK:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline
;
; OPTM1-LABEL: pass64BitConstArg
; OPTM1:      sub     esp
; OPTM1:      mov     DWORD PTR [esp+0x4]
; OPTM1:      mov     DWORD PTR [esp]
; OPTM1-NEXT: mov     DWORD PTR [esp+0x8],0x7b
; Bundle padding might be added (so not using -NEXT).
; OPTM1:      mov     DWORD PTR [esp+0x10],0xdeadbeef
; OPTM1-NEXT: mov     DWORD PTR [esp+0xc],0x12345678
; OPTM1-NOT:  mov
; OPTM1:      call {{.*}} R_{{.*}}    ignore64BitArgNoInline

; ARM32-LABEL: pass64BitConstArg
; ARM32:      movw    [[REG1:r.*]], {{.*}} ; 0xbeef
; ARM32:      movt    [[REG1]], {{.*}}     ; 0xdead
; ARM32:      movw    [[REG2:r.*]], {{.*}} ; 0x5678
; ARM32:      movt    [[REG2]], {{.*}}     ; 0x1234
; ARM32:      str     [[REG1]], [sp, #4]
; ARM32:      str     [[REG2]], [sp]
; ARM32:      {{mov|ldr}} r0
; ARM32:      {{mov|ldr}} r1
; ARM32:      mov     r2, #123
; ARM32:      bl      {{.*}} ignore64BitArgNoInline

; MIPS32-LABEL: pass64BitConstArg
; MIPS32-O2: 	lui	[[REG:.*]],0xdead
; MIPS32-O2: 	ori	[[REG1:.*]],[[REG]],0xbeef
; MIPS32-O2: 	lui	[[REG:.*]],0x1234
; MIPS32-O2: 	ori	[[REG2:.*]],[[REG]],0x5678
; MIPS32-O2: 	sw	[[REG1]],{{.*}}(sp)
; MIPS32-O2: 	sw	[[REG2]],{{.*}}(sp)
; MIPS32-O2: 	move	a0,a2
; MIPS32-O2: 	move	a1,a3
; MIPS32-O2: 	li	a2,123
; MIPS32-O2: 	jal	{{.*}}	ignore64BitArgNoInline

define internal i32 @pass64BitUndefArg() {
entry:
  %call = call i32 @ignore64BitArgNoInline(i64 0, i32 123, i64 undef)
  ret i32 %call
}
; CHECK-LABEL: pass64BitUndefArg
; CHECK: sub esp
; CHECK: mov DWORD PTR{{.*}},0x7b
; CHECK: mov DWORD PTR{{.*}},0x0
; CHECK: call {{.*}} R_{{.*}} ignore64BitArgNoInline
; OPTM1-LABEL: pass64BitUndefArg
; OPTM1: sub esp
; OPTM1: mov DWORD PTR{{.*}},0x7b
; OPTM1: mov DWORD PTR{{.*}},0x0
; OPTM1: call {{.*}} R_{{.*}} ignore64BitArgNoInline
; ARM32-LABEL: pass64BitUndefArg
; ARM32: sub sp
; ARM32: mov {{.*}}, #0
; ARM32: str
; ARM32: mov {{.*}}, #123
; ARM32: bl {{.*}} ignore64BitArgNoInline

; MIPS32-LABEL: pass64BitUndefArg
; MIPS32: jr  ra

define internal i64 @return64BitArg(i64 %padding, i64 %a) {
entry:
  ret i64 %a
}
; CHECK-LABEL: return64BitArg
; CHECK: mov     {{.*}},DWORD PTR [esp+0xc]
; CHECK: mov     {{.*}},DWORD PTR [esp+0x10]
;
; OPTM1-LABEL: return64BitArg
; OPTM1: mov     {{.*}},DWORD PTR [esp+0xc]
; OPTM1: mov     {{.*}},DWORD PTR [esp+0x10]

; ARM32-LABEL: return64BitArg
; ARM32: mov {{.*}}, r2
; ARM32: mov {{.*}}, r3
; ARM32: bx lr

; MIPS32-LABEL; return64BitArg
; MIPS32-O2: move v0,a2
; MIPS32-O2: move v1,a3
; MIPS32-OM1: move [[T1:.*]],a2
; MIPS32-OM1: sw [[T1]],[[MEM1:.*]]
; MIPS32-OM1: move [[T2:.*]],a3
; MIPS32-OM1: sw [[T2]],[[MEM2:.*]]
; MIPS32-OM1: lw v0,[[MEM1]]
; MIPS32-OM1: lw v1,[[MEM2]]
; MIPS32: jr ra

define internal i64 @return64BitConst() {
entry:
  ret i64 -2401053092306725256
}
; CHECK-LABEL: return64BitConst
; CHECK: mov     eax,0x12345678
; CHECK: mov     edx,0xdeadbeef
;
; OPTM1-LABEL: return64BitConst
; OPTM1: mov     eax,0x12345678
; OPTM1: mov     edx,0xdeadbeef

; ARM32-LABEL: return64BitConst
; ARM32: movw r0, #22136 ; 0x5678
; ARM32: movt r0, #4660  ; 0x1234
; ARM32: movw r1, #48879 ; 0xbeef
; ARM32: movt r1, #57005 ; 0xdead

; MIPS32-LABEL: return64BitConst
; MIPS32: lui v0,0x1234
; MIPS32: ori v0,v0,0x5678
; MIPS32: lui v1,0xdead
; MIPS32: ori v1,v1,0xbeef
; MIPS32: jr ra

define internal i64 @add64BitSigned(i64 %a, i64 %b) {
entry:
  %add = add i64 %b, %a
  ret i64 %add
}
; CHECK-LABEL: add64BitSigned
; CHECK: add
; CHECK: adc
;
; OPTM1-LABEL: add64BitSigned
; OPTM1: add
; OPTM1: adc

; ARM32-LABEL: add64BitSigned
; ARM32: adds
; ARM32: adc

; MIPS32-LABEL: add64BitSigned
; MIPS32: addu
; MIPS32: sltu
; MIPS32: addu
; MIPS32: addu

define internal i64 @add64BitUnsigned(i64 %a, i64 %b) {
entry:
  %add = add i64 %b, %a
  ret i64 %add
}
; CHECK-LABEL: add64BitUnsigned
; CHECK: add
; CHECK: adc
;
; OPTM1-LABEL: add64BitUnsigned
; OPTM1: add
; OPTM1: adc

; ARM32-LABEL: add64BitUnsigned
; ARM32: adds
; ARM32: adc

; MIPS32-LABEL: add64BitUnsigned
; MIPS32: addu
; MIPS32: sltu
; MIPS32: addu
; MIPS32: addu

define internal i64 @sub64BitSigned(i64 %a, i64 %b) {
entry:
  %sub = sub i64 %a, %b
  ret i64 %sub
}
; CHECK-LABEL: sub64BitSigned
; CHECK: sub
; CHECK: sbb
;
; OPTM1-LABEL: sub64BitSigned
; OPTM1: sub
; OPTM1: sbb

; ARM32-LABEL: sub64BitSigned
; ARM32: subs
; ARM32: sbc

; MIPS32-LABEL: sub64BitSigned
; MIPS32: subu
; MIPS32: sltu
; MIPS32: addu
; MIPS32: subu

define internal i64 @sub64BitUnsigned(i64 %a, i64 %b) {
entry:
  %sub = sub i64 %a, %b
  ret i64 %sub
}
; CHECK-LABEL: sub64BitUnsigned
; CHECK: sub
; CHECK: sbb
;
; OPTM1-LABEL: sub64BitUnsigned
; OPTM1: sub
; OPTM1: sbb

; ARM32-LABEL: sub64BitUnsigned
; ARM32: subs
; ARM32: sbc

; MIPS32-LABEL: sub64BitUnsigned
; MIPS32: subu
; MIPS32: sltu
; MIPS32: addu
; MIPS32: subu

define internal i64 @mul64BitSigned(i64 %a, i64 %b) {
entry:
  %mul = mul i64 %b, %a
  ret i64 %mul
}
; CHECK-LABEL: mul64BitSigned
; CHECK: imul
; CHECK: mul
; CHECK: add
; CHECK: imul
; CHECK: add
;
; OPTM1-LABEL: mul64BitSigned
; OPTM1: imul
; OPTM1: mul
; OPTM1: add
; OPTM1: imul
; OPTM1: add

; ARM32-LABEL: mul64BitSigned
; ARM32: mul
; ARM32: mla
; ARM32: umull
; ARM32: add

; MIPS32-LABEL: mul64BitSigned
; MIPS32: multu
; MIPS32: mflo
; MIPS32: mfhi
; MIPS32: mul
; MIPS32: mul
; MIPS32: addu
; MIPS32: addu

define internal i64 @mul64BitUnsigned(i64 %a, i64 %b) {
entry:
  %mul = mul i64 %b, %a
  ret i64 %mul
}
; CHECK-LABEL: mul64BitUnsigned
; CHECK: imul
; CHECK: mul
; CHECK: add
; CHECK: imul
; CHECK: add
;
; OPTM1-LABEL: mul64BitUnsigned
; OPTM1: imul
; OPTM1: mul
; OPTM1: add
; OPTM1: imul
; OPTM1: add

; ARM32-LABEL: mul64BitUnsigned
; ARM32: mul
; ARM32: mla
; ARM32: umull
; ARM32: add

; MIPS32-LABEL: mul64BitUnsigned
; MIPS32: multu
; MIPS32: mflo
; MIPS32: mfhi
; MIPS32: mul
; MIPS32: mul
; MIPS32: addu
; MIPS32: addu

define internal i64 @div64BitSigned(i64 %a, i64 %b) {
entry:
  %div = sdiv i64 %a, %b
  ret i64 %div
}
; CHECK-LABEL: div64BitSigned
; CHECK: call {{.*}} R_{{.*}}    __divdi3

; OPTM1-LABEL: div64BitSigned
; OPTM1: call {{.*}} R_{{.*}}    __divdi3
;
; ARM32-LABEL: div64BitSigned
; ARM32: orrs {{r.*}}, {{r.*}}
; ARM32: bne
; ARM32: bl {{.*}} __divdi3

; MIPS32-LABEL: div64BitSigned
; MIPS32: jal {{.*}} __divdi3

define internal i64 @div64BitSignedConst(i64 %a) {
entry:
  %div = sdiv i64 %a, 12345678901234
  ret i64 %div
}
; CHECK-LABEL: div64BitSignedConst
; CHECK: mov     DWORD PTR [esp+0xc],0xb3a
; CHECK: mov     DWORD PTR [esp+0x8],0x73ce2ff2
; CHECK: call {{.*}} R_{{.*}}    __divdi3
;
; OPTM1-LABEL: div64BitSignedConst
; OPTM1: mov     DWORD PTR [esp+0xc],0xb3a
; OPTM1: mov     DWORD PTR [esp+0x8],0x73ce2ff2
; OPTM1: call {{.*}} R_{{.*}}    __divdi3
;
; ARM32-LABEL: div64BitSignedConst
; For a constant, we should be able to optimize-out the divide by zero check.
; ARM32-NOT: orrs
; ARM32: movw {{.*}} ; 0x2ff2
; ARM32: movt {{.*}} ; 0x73ce
; ARM32: movw {{.*}} ; 0xb3a
; ARM32: bl {{.*}} __divdi3

; MIPS32-LABEL: div64BitSignedConst
; MIPS32: jal {{.*}} __divdi3

define internal i64 @div64BitUnsigned(i64 %a, i64 %b) {
entry:
  %div = udiv i64 %a, %b
  ret i64 %div
}
; CHECK-LABEL: div64BitUnsigned
; CHECK: call {{.*}} R_{{.*}}    __udivdi3
;
; OPTM1-LABEL: div64BitUnsigned
; OPTM1: call {{.*}} R_{{.*}}    __udivdi3
;
; ARM32-LABEL: div64BitUnsigned
; ARM32: orrs {{r.*}}, {{r.*}}
; ARM32: bne
; ARM32: bl {{.*}} __udivdi3

; MIPS32-LABEL: div64BitUnsigned
; MIPS32: jal {{.*}} __udivdi3

define internal i64 @rem64BitSigned(i64 %a, i64 %b) {
entry:
  %rem = srem i64 %a, %b
  ret i64 %rem
}
; CHECK-LABEL: rem64BitSigned
; CHECK: call {{.*}} R_{{.*}}    __moddi3
;
; OPTM1-LABEL: rem64BitSigned
; OPTM1: call {{.*}} R_{{.*}}    __moddi3
;
; ARM32-LABEL: rem64BitSigned
; ARM32: orrs {{r.*}}, {{r.*}}
; ARM32: bne
; ARM32: bl {{.*}} __moddi3

; MIPS32-LABEL: rem64BitSigned
; MIPS32: jal {{.*}} __moddi3

define internal i64 @rem64BitUnsigned(i64 %a, i64 %b) {
entry:
  %rem = urem i64 %a, %b
  ret i64 %rem
}
; CHECK-LABEL: rem64BitUnsigned
; CHECK: call {{.*}} R_{{.*}}    __umoddi3
;
; OPTM1-LABEL: rem64BitUnsigned
; OPTM1: call {{.*}} R_{{.*}}    __umoddi3
;
; ARM32-LABEL: rem64BitUnsigned
; ARM32: orrs {{r.*}}, {{r.*}}
; ARM32: bne
; ARM32: bl {{.*}} __umoddi3

; MIPS32-LABEL: rem64BitUnsigned
; MIPS32: jal {{.*}} __umoddi3

define internal i64 @shl64BitSigned(i64 %a, i64 %b) {
entry:
  %shl = shl i64 %a, %b
  ret i64 %shl
}
; CHECK-LABEL: shl64BitSigned
; CHECK: shld
; CHECK: shl e
; CHECK: test {{.*}},0x20
; CHECK: je
;
; OPTM1-LABEL: shl64BitSigned
; OPTM1: shld
; OPTM1: shl e
; OPTM1: test {{.*}},0x20
; OPTM1: je

; ARM32-LABEL: shl64BitSigned
; ARM32: rsb     [[T0:r[0-9]+]], r2, #32
; ARM32: lsr     [[T1:r[0-9]+]], r0, [[T0]]
; ARM32: orr     [[T2:r[0-9]+]], [[T1]], r1, lsl r2
; ARM32: sub     [[T3:r[0-9]+]], r2, #32
; ARM32: cmp     [[T3]], #0
; ARM32: lslge   [[T2]], r0, [[T3]]
; ARM32: lsl     r{{[0-9]+}}, r0, r2

; MIPS32-LABEL: shl64BitSigned
; MIPS32: sllv	[[T1:.*]],[[A_HI:.*]],[[B_LO:.*]]
; MIPS32: nor	[[T2:.*]],[[B_LO]],zero
; MIPS32: srl	[[T3:.*]],[[A_LO:.*]],0x1
; MIPS32: srlv	[[T4:.*]],[[T3]],[[T2]]
; MIPS32: or	[[T_HI:.*]],[[T1]],[[T4]]
; MIPS32: sllv	[[T_LO:.*]],[[A_LO]],[[B_LO]]
; MIPS32: move	[[T1_LO:.*]],[[T_LO]]
; MIPS32: andi	[[T5:.*]],[[B_LO]],0x20
; MIPS32: movn	[[T_HI]],[[T_LO]],[[T5]]
; MIPS32: movn	[[T1_LO]],zero,[[T5]]
; MIPS32-O2: move	v1,[[T_HI]]
; MIPS32-OM1: sw	[[T_HI]],[[MEM:.*]]
; MIPS32-OM1: lw        v1,[[MEM]]

define internal i32 @shl64BitSignedTrunc(i64 %a, i64 %b) {
entry:
  %shl = shl i64 %a, %b
  %result = trunc i64 %shl to i32
  ret i32 %result
}
; CHECK-LABEL: shl64BitSignedTrunc
; CHECK: mov
; CHECK: shl e
; CHECK: test {{.*}},0x20
; CHECK: je
;
; OPTM1-LABEL: shl64BitSignedTrunc
; OPTM1: shld
; OPTM1: shl e
; OPTM1: test {{.*}},0x20
; OPTM1: je

; ARM32-LABEL: shl64BitSignedTrunc
; ARM32: lsl r

; MIPS32-LABEL: shl64BitSignedTrunc
; MIPS32-O2: 	sllv
; MIPS32-O2: 	andi	{{.*}},0x20
; MIPS32-O2: 	movn

define internal i64 @shl64BitUnsigned(i64 %a, i64 %b) {
entry:
  %shl = shl i64 %a, %b
  ret i64 %shl
}
; CHECK-LABEL: shl64BitUnsigned
; CHECK: shld
; CHECK: shl e
; CHECK: test {{.*}},0x20
; CHECK: je
;
; OPTM1-LABEL: shl64BitUnsigned
; OPTM1: shld
; OPTM1: shl e
; OPTM1: test {{.*}},0x20
; OPTM1: je

; ARM32-LABEL: shl64BitUnsigned
; ARM32: rsb
; ARM32: lsr
; ARM32: orr
; ARM32: sub
; ARM32: cmp
; ARM32: lslge
; ARM32: lsl

; MIPS32-LABEL: shl64BitUnsigned
; MIPS32: sllv  [[T1:.*]],[[A_HI:.*]],[[B_LO:.*]]
; MIPS32: nor   [[T2:.*]],[[B_LO]],zero
; MIPS32: srl   [[T3:.*]],[[A_LO:.*]],0x1
; MIPS32: srlv  [[T4:.*]],[[T3]],[[T2]]
; MIPS32: or    [[T_HI:.*]],[[T1]],[[T4]]
; MIPS32: sllv  [[T_LO:.*]],[[A_LO]],[[B_LO]]
; MIPS32: move  [[T1_LO:.*]],[[T_LO]]
; MIPS32: andi  [[T5:.*]],[[B_LO]],0x20
; MIPS32: movn  [[T_HI]],[[T_LO]],[[T5]]
; MIPS32: movn  [[T1_LO]],zero,[[T5]]
; MIPS32-O2: move       v1,[[T_HI]]
; MIPS32-OM1: sw        [[T_HI]],[[MEM:.*]]
; MIPS32-OM1: lw        v1,[[MEM]]

define internal i64 @shr64BitSigned(i64 %a, i64 %b) {
entry:
  %shr = ashr i64 %a, %b
  ret i64 %shr
}
; CHECK-LABEL: shr64BitSigned
; CHECK: shrd
; CHECK: sar
; CHECK: test {{.*}},0x20
; CHECK: je
; CHECK: sar {{.*}},0x1f
;
; OPTM1-LABEL: shr64BitSigned
; OPTM1: shrd
; OPTM1: sar
; OPTM1: test {{.*}},0x20
; OPTM1: je
; OPTM1: sar {{.*}},0x1f

; ARM32-LABEL: shr64BitSigned
; ARM32: lsr     [[T0:r[0-9]+]], r{{[0-9]+}}, r{{[0-9]+}}
; ARM32: rsb     [[T1:r[0-9]+]], r{{[0-9]+}}, #32
; ARM32: orr     r{{[0-9]+}}, [[T0]], r{{[0-9]+}}, lsl [[T1]]
; ARM32: sub     [[T2:r[0-9]+]], r{{[0-9]+}}, #32
; ARM32: cmp     [[T2]], #0
; ARM32: asrge   r{{[0-9]+}}, r{{[0-9]+}}, [[T2]]
; ARM32: asr     r{{[0-9]+}}, r{{[0-9]+}}, r{{[0-9]+}}

; MIPS32-LABEL: shr64BitSigned
; MIPS32: srlv	[[T1:.*]],[[A_LO:.*]],[[B_LO:.*]]
; MIPS32: nor	[[T2:.*]],[[B_LO]],zero
; MIPS32: sll	[[T3:.*]],[[A_HI:.*]],0x1
; MIPS32: sllv	[[T4:.*]],[[T3]],[[T2]]
; MIPS32: or	[[T_LO:.*]],[[T1]],[[T4]]
; MIPS32: srav	[[T_HI:.*]],[[A_HI]],[[B_LO]]
; MIPS32: move	[[T_HI1:.*]],[[T_HI]]
; MIPS32: andi	[[T5:.*]],[[B_LO]],0x20
; MIPS32: movn	[[T_LO1:.*]],[[T_HI]],[[T5]]
; MIPS32: sra	[[T6:.*]],[[A_HI]],0x1f
; MIPS32: movn	[[T_HI1]],[[T6]],[[T5]]

define internal i32 @shr64BitSignedTrunc(i64 %a, i64 %b) {
entry:
  %shr = ashr i64 %a, %b
  %result = trunc i64 %shr to i32
  ret i32 %result
}
; CHECK-LABEL: shr64BitSignedTrunc
; CHECK: shrd
; CHECK: sar
; CHECK: test {{.*}},0x20
; CHECK: je
;
; OPTM1-LABEL: shr64BitSignedTrunc
; OPTM1: shrd
; OPTM1: sar
; OPTM1: test {{.*}},0x20
; OPTM1: je
; OPTM1: sar {{.*}},0x1f

; ARM32-LABEL: shr64BitSignedTrunc
; ARM32: lsr
; ARM32: rsb
; ARM32: orr
; ARM32: sub
; ARM32: cmp
; ARM32: asrge

; MIPS32-LABEL: shr64BitSignedTrunc
; MIPS32-O2: 	srlv
; MIPS32-O2: 	nor
; MIPS32-O2: 	sll
; MIPS32-O2: 	sllv
; MIPS32-O2: 	or
; MIPS32-O2: 	srav
; MIPS32-O2: 	andi	{{.*}},0x20
; MIPS32-O2: 	movn

define internal i64 @shr64BitUnsigned(i64 %a, i64 %b) {
entry:
  %shr = lshr i64 %a, %b
  ret i64 %shr
}
; CHECK-LABEL: shr64BitUnsigned
; CHECK: shrd
; CHECK: shr
; CHECK: test {{.*}},0x20
; CHECK: je
;
; OPTM1-LABEL: shr64BitUnsigned
; OPTM1: shrd
; OPTM1: shr
; OPTM1: test {{.*}},0x20
; OPTM1: je

; ARM32-LABEL: shr64BitUnsigned
; ARM32: lsr
; ARM32: rsb
; ARM32: orr
; ARM32: sub
; ARM32: cmp
; ARM32: lsrge
; ARM32: lsr

; MIPS32-LABEL: shr64BitUnsigned
; MIPS32: srlv  [[T1:.*]],[[A_LO:.*]],[[B_LO:.*]]
; MIPS32: nor   [[T2:.*]],[[B_LO]],zero
; MIPS32: sll   [[T3:.*]],[[A_HI:.*]],0x1
; MIPS32: sllv  [[T4:.*]],[[T3]],[[T2]]
; MIPS32: or    [[T_LO:.*]],[[T1]],[[T4]]
; MIPS32: srlv  [[T_HI:.*]],[[A_HI]],[[B_LO]]
; MIPS32: move  [[T_HI1:.*]],[[T_HI]]
; MIPS32: andi  [[T5:.*]],[[B_LO]],0x20
; MIPS32: movn  [[T_LO1:.*]],[[T_HI]],[[T5]]
; MIPS32: movn  [[T_HI1]],zero,[[T5]]

define internal i32 @shr64BitUnsignedTrunc(i64 %a, i64 %b) {
entry:
  %shr = lshr i64 %a, %b
  %result = trunc i64 %shr to i32
  ret i32 %result
}
; CHECK-LABEL: shr64BitUnsignedTrunc
; CHECK: shrd
; CHECK: shr
; CHECK: test {{.*}},0x20
; CHECK: je
;
; OPTM1-LABEL: shr64BitUnsignedTrunc
; OPTM1: shrd
; OPTM1: shr
; OPTM1: test {{.*}},0x20
; OPTM1: je

; ARM32-LABEL: shr64BitUnsignedTrunc
; ARM32: lsr
; ARM32: rsb
; ARM32: orr
; ARM32: sub
; ARM32: cmp
; ARM32: lsrge

; MIPS32-LABEL: shr64BitUnsignedTrunc
; MIPS32-O2: 	srlv
; MIPS32-O2: 	nor
; MIPS32-O2: 	sll
; MIPS32-O2: 	sllv
; MIPS32-O2: 	or
; MIPS32-O2: 	srlv
; MIPS32-O2: 	andi
; MIPS32-O2: 	movn

define internal i64 @and64BitSigned(i64 %a, i64 %b) {
entry:
  %and = and i64 %b, %a
  ret i64 %and
}
; CHECK-LABEL: and64BitSigned
; CHECK: and
; CHECK: and
;
; OPTM1-LABEL: and64BitSigned
; OPTM1: and
; OPTM1: and

; ARM32-LABEL: and64BitSigned
; ARM32: and
; ARM32: and

; MIPS32-LABEL: and64BitSigned
; MIPS32: and
; MIPS32: and

define internal i64 @and64BitUnsigned(i64 %a, i64 %b) {
entry:
  %and = and i64 %b, %a
  ret i64 %and
}
; CHECK-LABEL: and64BitUnsigned
; CHECK: and
; CHECK: and
;
; OPTM1-LABEL: and64BitUnsigned
; OPTM1: and
; OPTM1: and

; ARM32-LABEL: and64BitUnsigned
; ARM32: and
; ARM32: and

; MIPS32-LABEL: and64BitUnsigned
; MIPS32: and
; MIPS32: and

define internal i64 @or64BitSigned(i64 %a, i64 %b) {
entry:
  %or = or i64 %b, %a
  ret i64 %or
}
; CHECK-LABEL: or64BitSigned
; CHECK: or
; CHECK: or
;
; OPTM1-LABEL: or64BitSigned
; OPTM1: or
; OPTM1: or

; ARM32-LABEL: or64BitSigned
; ARM32: orr
; ARM32: orr

; MIPS32-LABEL: or64BitSigned
; MIPS32: or
; MIPS32: or

define internal i64 @or64BitUnsigned(i64 %a, i64 %b) {
entry:
  %or = or i64 %b, %a
  ret i64 %or
}
; CHECK-LABEL: or64BitUnsigned
; CHECK: or
; CHECK: or
;
; OPTM1-LABEL: or64BitUnsigned
; OPTM1: or
; OPTM1: or

; ARM32-LABEL: or64BitUnsigned
; ARM32: orr
; ARM32: orr

; MIPS32-LABEL: or64BitUnsigned
; MIPS32: or
; MIPS32: or

define internal i64 @xor64BitSigned(i64 %a, i64 %b) {
entry:
  %xor = xor i64 %b, %a
  ret i64 %xor
}
; CHECK-LABEL: xor64BitSigned
; CHECK: xor
; CHECK: xor
;
; OPTM1-LABEL: xor64BitSigned
; OPTM1: xor
; OPTM1: xor

; ARM32-LABEL: xor64BitSigned
; ARM32: eor
; ARM32: eor

; MIPS32-LABEL: xor64BitSigned
; MIPS32: xor
; MIPS32: xor

define internal i64 @xor64BitUnsigned(i64 %a, i64 %b) {
entry:
  %xor = xor i64 %b, %a
  ret i64 %xor
}
; CHECK-LABEL: xor64BitUnsigned
; CHECK: xor
; CHECK: xor
;
; OPTM1-LABEL: xor64BitUnsigned
; OPTM1: xor
; OPTM1: xor

; ARM32-LABEL: xor64BitUnsigned
; ARM32: eor
; ARM32: eor

; MIPS32-LABEL: xor64BitUnsigned
; MIPS32: xor
; MIPS32: xor

define internal i32 @trunc64To32Signed(i64 %padding, i64 %a) {
entry:
  %conv = trunc i64 %a to i32
  ret i32 %conv
}
; CHECK-LABEL: trunc64To32Signed
; CHECK: mov     eax,DWORD PTR [esp+0xc]
;
; OPTM1-LABEL: trunc64To32Signed
; OPTM1: mov     eax,DWORD PTR [esp+

; ARM32-LABEL: trunc64To32Signed
; ARM32: mov r0, r2

; MIPS32-LABEL: trunc64To32Signed
; MIPS32: move v0,a2

define internal i32 @trunc64To16Signed(i64 %a) {
entry:
  %conv = trunc i64 %a to i16
  %conv.ret_ext = sext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: trunc64To16Signed
; CHECK:      mov     eax,DWORD PTR [esp+0x4]
; CHECK-NEXT: movsx  eax,ax
;
; OPTM1-LABEL: trunc64To16Signed
; OPTM1:      mov     ax,WORD PTR [esp+
; OPTM1: movsx  eax,

; ARM32-LABEL: trunc64To16Signed
; ARM32: sxth r0, r0

; MIPS32-LABEL: trunc64To16Signed
; MIPS32-O2: sll [[T1:.*]],a0,0x10
; MIPS32-O2: sra [[T2:.*]],[[T1]],0x10
; MIPS32-O2: move v0,[[T2]]
; MIPS32-OM1: sll [[T1:.*]],{{.*}},0x10
; MIPS32-OM1: sra [[T2:.*]],[[T1]],0x10
; MIPS32-OM1: sw [[T2]],[[MEM:.*]]
; MIPS32-OM1: lw v0,[[MEM]]

define internal i32 @trunc64To8Signed(i64 %a) {
entry:
  %conv = trunc i64 %a to i8
  %conv.ret_ext = sext i8 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: trunc64To8Signed
; CHECK:      mov     eax,DWORD PTR [esp+0x4]
; CHECK-NEXT: movsx  eax,al
;
; OPTM1-LABEL: trunc64To8Signed
; OPTM1:      mov     eax,DWORD PTR [esp+
; OPTM1: movsx  eax,

; ARM32-LABEL: trunc64To8Signed
; ARM32: sxtb r0, r0

; MIPS32-LABEL: trunc64To8Signed
; MIPS32-O2: sll [[T1:.*]],a0,0x18
; MIPS32-O2: sra [[T2:.*]],[[T1]],0x18
; MIPS32-O2: move v0,[[T2]]
; MIPS32-OM1: sll [[T1:.*]],{{.*}},0x18
; MIPS32-OM1: sra [[T2:.*]],[[T1]],0x18
; MIPS32-OM1: sw [[T2]],[[MEM:.*]]
; MIPS32-OM1: lw v0,[[MEM]]

define internal i32 @trunc64To32SignedConst() {
entry:
  %conv = trunc i64 12345678901234 to i32
  ret i32 %conv
}
; CHECK-LABEL: trunc64To32SignedConst
; CHECK: mov eax,0x73ce2ff2
;
; OPTM1-LABEL: trunc64To32SignedConst
; OPTM1: mov eax,0x73ce2ff2

; ARM32-LABEL: trunc64To32SignedConst
; ARM32: movw r0, #12274 ; 0x2ff2
; ARM32: movt r0, #29646 ; 0x73ce

; MIPS32-LABEL: trunc64To32SignedConst
; MIPS32: lui v0,0x73ce
; MIPS32: ori v0,v0,0x2ff2

define internal i32 @trunc64To16SignedConst() {
entry:
  %conv = trunc i64 12345678901234 to i16
  %conv.ret_ext = sext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: trunc64To16SignedConst
; CHECK: mov eax,0x73ce2ff2
; CHECK: movsx eax,ax
;
; OPTM1-LABEL: trunc64To16SignedConst
; OPTM1: mov eax,0x73ce2ff2
; OPTM1: movsx eax,

; ARM32-LABEL: trunc64To16SignedConst
; ARM32: movw r0, #12274 ; 0x2ff2
; ARM32: movt r0, #29646 ; 0x73ce
; ARM32: sxth r0, r0

; MIPS32-LABEL: trunc64To16SignedConst
; MIPS32: lui v0,0x73ce
; MIPS32: ori v0,v0,0x2ff2
; MIPS32: sll v0,v0,0x10
; MIPS32: sra v0,v0,0x10

define internal i32 @trunc64To32Unsigned(i64 %padding, i64 %a) {
entry:
  %conv = trunc i64 %a to i32
  ret i32 %conv
}
; CHECK-LABEL: trunc64To32Unsigned
; CHECK: mov     eax,DWORD PTR [esp+0xc]
;
; OPTM1-LABEL: trunc64To32Unsigned
; OPTM1: mov     eax,DWORD PTR [esp+

; ARM32-LABEL: trunc64To32Unsigned
; ARM32: mov r0, r2

; MIPS32-LABEL: trunc64To32Unsigned
; MIPS32: move v0,a2

define internal i32 @trunc64To16Unsigned(i64 %a) {
entry:
  %conv = trunc i64 %a to i16
  %conv.ret_ext = zext i16 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: trunc64To16Unsigned
; CHECK:      mov     eax,DWORD PTR [esp+0x4]
; CHECK-NEXT: movzx  eax,ax
;
; OPTM1-LABEL: trunc64To16Unsigned
; OPTM1:      mov     ax,WORD PTR [esp+
; OPTM1: movzx  eax,

; ARM32-LABEL: trunc64To16Unsigned
; ARM32: uxth

; MIPS32-LABEL: trunc64To16Unsigned
; MIPS32-O2: andi [[T1:.*]],a0,0xffff
; MIPS32-O2: move v0,[[T1]]
; MIPS32-OM1: andi [[T1:.*]],{{.*}},0xffff
; MIPS32-OM1: sw [[T1]],[[MEM:.*]]
; MIPS32-OM1: lw v0,[[MEM]]

define internal i32 @trunc64To8Unsigned(i64 %a) {
entry:
  %conv = trunc i64 %a to i8
  %conv.ret_ext = zext i8 %conv to i32
  ret i32 %conv.ret_ext
}
; CHECK-LABEL: trunc64To8Unsigned
; CHECK:      mov     eax,DWORD PTR [esp+0x4]
; CHECK-NEXT: movzx  eax,al
;
; OPTM1-LABEL: trunc64To8Unsigned
; OPTM1: mov    eax,DWORD PTR [esp+
; OPTM1: movzx  eax,

; ARM32-LABEL: trunc64To8Unsigned
; ARM32: uxtb

; MIPS32-LABEL: trunc64To8Unsigned
; MIPS32-O2: andi [[T1:.*]],a0,0xff
; MIPS32-O2: move v0,[[T1]]
; MIPS32-OM1: andi [[T1:.*]],{{.*}},0xff
; MIPS32-OM1: sw [[T1]],[[MEM:.*]]
; MIPS32-OM1: lw v0,[[MEM]]

define internal i32 @trunc64To1(i64 %a) {
entry:
;  %tobool = icmp ne i64 %a, 0
  %tobool = trunc i64 %a to i1
  %tobool.ret_ext = zext i1 %tobool to i32
  ret i32 %tobool.ret_ext
}
; CHECK-LABEL: trunc64To1
; CHECK:      mov     eax,DWORD PTR [esp+0x4]
; CHECK:      and     al,0x1
; CHECK-NOT:  and     eax,0x1
;
; OPTM1-LABEL: trunc64To1
; OPTM1:      mov     eax,DWORD PTR [esp+
; OPTM1:      and     al,0x1
; OPTM1-NOT:  and     eax,0x1

; ARM32-LABEL: trunc64To1
; ARM32-OM1: and r0, r0, #1
; ARM32-O2: and r0, r0, #1

; MIPS32-LABEL: trunc64To1
; MIPS32-O2: andi [[T1:.*]],a0,0x1
; MIPS32-O2: move v0,[[T1]]
; MIPS32-OM1: andi [[T1:.*]],{{.*}},0x1
; MIPS32-OM1: sw [[T1]],[[MEM:.*]]
; MIPS32-OM1: lw v0,[[MEM]]

define internal i64 @sext32To64(i32 %a) {
entry:
  %conv = sext i32 %a to i64
  ret i64 %conv
}
; CHECK-LABEL: sext32To64
; CHECK: mov
; CHECK: sar {{.*}},0x1f
;
; OPTM1-LABEL: sext32To64
; OPTM1: mov
; OPTM1: sar {{.*}},0x1f

; ARM32-LABEL: sext32To64
; ARM32: asr {{.*}}, #31

; MIPS32-LABEL: sext32To64
; MIPS32: sra [[T_HI:.*]],[[T_LO:.*]],0x1f
; MIPS32-O2: move v1,[[T_HI]]
; MIPS32-O2: move v0,[[T_LO]]
; MIPS32-OM1: sw [[T_HI]],[[MEM_HI:.*]]
; MIPS32-OM1: sw [[T_LO]],[[MEM_LO:.*]]
; MIPS32-OM1: lw v0,[[MEM_LO]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal i64 @sext16To64(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i16
  %conv = sext i16 %a.arg_trunc to i64
  ret i64 %conv
}
; CHECK-LABEL: sext16To64
; CHECK: movsx
; CHECK: sar {{.*}},0x1f
;
; OPTM1-LABEL: sext16To64
; OPTM1: movsx
; OPTM1: sar {{.*}},0x1f

; ARM32-LABEL: sext16To64
; ARM32: sxth
; ARM32: asr {{.*}}, #31

; MIPS32-LABEL: sext16To64
; MIPS32: sll [[T1_LO:.*]],{{.*}},0x10
; MIPS32: sra [[T2_LO:.*]],[[T1_LO]],0x10
; MIPS32: sra [[T_HI:.*]],[[T2_LO]],0x1f
; MIPS32-O2: move v1,[[T_HI]]
; MIPS32-O2: move v0,[[T2_LO]]
; MIPS32-OM1: sw [[T_HI]],[[MEM_HI:.*]]
; MIPS32-OM1: sw [[T2_LO]],[[MEM_LO:.*]]
; MIPS32-OM1: lw v0,[[MEM_LO]]
; MIPS32-OM1: lw v1,[[MEM_HI]]


define internal i64 @sext8To64(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i8
  %conv = sext i8 %a.arg_trunc to i64
  ret i64 %conv
}
; CHECK-LABEL: sext8To64
; CHECK: movsx
; CHECK: sar {{.*}},0x1f
;
; OPTM1-LABEL: sext8To64
; OPTM1: movsx
; OPTM1: sar {{.*}},0x1f

; ARM32-LABEL: sext8To64
; ARM32: sxtb
; ARM32: asr {{.*}}, #31

; MIPS32-LABEL: sext8To64
; MIPS32: sll [[T1_LO:.*]],{{.*}},0x18
; MIPS32: sra [[T2_LO:.*]],[[T1_LO]],0x18
; MIPS32: sra [[T_HI:.*]],[[T2_LO]],0x1f
; MIPS32-O2: move v1,[[T_HI]]
; MIPS32-O2: move v0,[[T2_LO]]
; MIPS32-OM1: sw [[T_HI]],[[MEM_HI:.*]]
; MIPS32-OM1: sw [[T2_LO]],[[MEM_LO:.*]]
; MIPS32-OM1: lw v0,[[MEM_LO]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal i64 @sext1To64(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i1
  %conv = sext i1 %a.arg_trunc to i64
  ret i64 %conv
}
; CHECK-LABEL: sext1To64
; CHECK: mov
; CHECK: shl {{.*}},0x1f
; CHECK: sar {{.*}},0x1f
;
; OPTM1-LABEL: sext1To64
; OPTM1: mov
; OPTM1: shl {{.*}},0x1f
; OPTM1: sar {{.*}},0x1f

; ARM32-LABEL: sext1To64
; ARM32: mov {{.*}}, #0
; ARM32: tst {{.*}}, #1
; ARM32: mvn {{.*}}, #0
; ARM32: movne

; MIPS32-LABEL: sext1To64
; MIPS32: sll [[T1:.*]],{{.*}},0x1f
; MIPS32: sra [[T2:.*]],[[T1]],0x1f
; MIPS32-O2: move v1,[[T2]]
; MIPS32-O2: move v0,[[T2]]
; MIPS32-OM1: sw [[T2]],[[MEM_HI:.*]]
; MIPS32-OM1: sw [[T2]],[[MEM_LO:.*]]
; MIPS32-OM1: lw v0,[[MEM_LO]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal i64 @zext32To64(i32 %a) {
entry:
  %conv = zext i32 %a to i64
  ret i64 %conv
}
; CHECK-LABEL: zext32To64
; CHECK: mov
; CHECK: mov {{.*}},0x0
;
; OPTM1-LABEL: zext32To64
; OPTM1: mov
; OPTM1: mov {{.*}},0x0

; ARM32-LABEL: zext32To64
; ARM32: mov {{.*}}, #0

; MIPS32-LABEL: zext32To64
; MIPS32: li [[T1:.*]],0
; MIPS32-O2: move v1,[[T1]]
; MIPS32-O2: move v0,a0
; MIPS32-OM1: sw [[T1]],[[MEM_HI:.*]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal i64 @zext16To64(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i16
  %conv = zext i16 %a.arg_trunc to i64
  ret i64 %conv
}
; CHECK-LABEL: zext16To64
; CHECK: movzx
; CHECK: mov {{.*}},0x0
;
; OPTM1-LABEL: zext16To64
; OPTM1: movzx
; OPTM1: mov {{.*}},0x0

; ARM32-LABEL: zext16To64
; ARM32: uxth
; ARM32: mov {{.*}}, #0

; MIPS32-LABEL: zext16To64
; MIPS32: andi [[T_LO:.*]],{{.*}},0xffff
; MIPS32: li [[T_HI:.*]],0
; MIPS32-O2: move v1,[[T_HI]]
; MIPS32-O2: move v0,[[T_LO]]
; MIPS32-OM1: sw [[T_HI]],[[MEM_HI:.*]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal i64 @zext8To64(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i8
  %conv = zext i8 %a.arg_trunc to i64
  ret i64 %conv
}
; CHECK-LABEL: zext8To64
; CHECK: movzx
; CHECK: mov {{.*}},0x0
;
; OPTM1-LABEL: zext8To64
; OPTM1: movzx
; OPTM1: mov {{.*}},0x0

; ARM32-LABEL: zext8To64
; ARM32: uxtb
; ARM32: mov {{.*}}, #0

; MIPS32-LABEL: zext8To64
; MIPS32: andi [[T_LO:.*]],{{.*}},0xff
; MIPS32: li [[T_HI:.*]],0
; MIPS32-O2: move v1,[[T_HI]]
; MIPS32-O2: move v0,[[T_LO]]
; MIPS32-OM1: sw [[T_HI]],[[MEM_HI:.*]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal i64 @zext1To64(i32 %a) {
entry:
  %a.arg_trunc = trunc i32 %a to i1
  %conv = zext i1 %a.arg_trunc to i64
  ret i64 %conv
}
; CHECK-LABEL: zext1To64
; CHECK: and {{.*}},0x1
; CHECK: mov {{.*}},0x0
;
; OPTM1-LABEL: zext1To64
; OPTM1: and {{.*}},0x1
; OPTM1: mov {{.*}},0x0

; ARM32-LABEL: zext1To64
; ARM32: and {{.*}}, #1
; ARM32: mov {{.*}}, #0
; ARM32: bx

; MIPS32-LABEL: zext1To64
; MIPS32: andi [[T_LO:.*]],{{.*}},0x1
; MIPS32: li [[T_HI:.*]],0
; MIPS32-O2: move v1,[[T_HI]]
; MIPS32-O2: move v0,[[T_LO]]
; MIPS32-OM1: sw [[T_HI]],[[MEM_HI:.*]]
; MIPS32-OM1: lw v1,[[MEM_HI]]

define internal void @icmpEq64(i64 %a, i64 %b, i64 %c, i64 %d) {
entry:
  %cmp = icmp eq i64 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp eq i64 %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: icmpEq64
; CHECK: cmp {{.*}}
; CHECK-NEXT: jne {{.*}}
; CHECK-NEXT: cmp {{.*}}
; CHECK-NEXT: jne {{.*}}
; CHECK-NEXT: call {{.*}}
; CHECK: cmp {{.*}}
; CHECK-NEXT: jne {{.*}}
; CHECK-NEXT: cmp {{.*}}
; CHECK-NEXT: jne {{.*}}
; CHECK-NEXT: call {{.*}}
;
; OPTM1-LABEL: icmpEq64
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: je {{.*}}
; OPTM1-NEXT: mov [[RESULT]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0
; OPTM1-NEXT: jne
; OPTM1-NEXT: jmp
; OPTM1-NEXT: call
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: je {{.*}}
; OPTM1-NEXT: mov [[RESULT]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0
; OPTM1-NEXT: jne
; OPTM1-NEXT: jmp
; OPTM1-NEXT: call

; ARM32-LABEL: icmpEq64
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32: bne
; ARM32: bl {{.*}} <func>
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32: bne
; ARM32: bl {{.*}} <func>
; ARM32: bx

; MIPS32-LABEL: icmpEq64
; MIPS32: xor [[T1:.*]],{{.*}},{{.*}}
; MIPS32: xor [[T2:.*]],{{.*}},{{.*}}
; MIPS32: or [[T3:.*]],[[T1]],[[T2]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sltiu [[T4:.*]],[[T3]],1
; MIPS32-OM1: sw [[T4]],[[MEM:.*]]
; MIPS32-OM1: lb [[T5:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T5]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: xor [[T1:.*]],{{.*}},{{.*}}
; MIPS32: xor [[T2:.*]],{{.*}},{{.*}}
; MIPS32: or [[T3:.*]],[[T1]],[[T2]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sltiu [[T4:.*]],[[T3]],1
; MIPS32-OM1: sw [[T4]],[[MEM:.*]]
; MIPS32-OM1: lb [[T5:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T5]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}

declare void @func()

define internal void @icmpNe64(i64 %a, i64 %b, i64 %c, i64 %d) {
entry:
  %cmp = icmp ne i64 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp ne i64 %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.end, %if.then2
  ret void
}
; CHECK-LABEL: icmpNe64
; CHECK: cmp {{.*}}
; CHECK-NEXT: jne {{.*}}
; CHECK-NEXT: cmp {{.*}}
; CHECK-NEXT: je {{.*}}
; CHECK-NEXT: call {{.*}}
; CHECK: cmp {{.*}}
; CHECK-NEXT: jne {{.*}}
; CHECK-NEXT: cmp {{.*}}
; CHECK-NEXT: je {{.*}}
; CHECK-NEXT: call {{.*}}
;
; OPTM1-LABEL: icmpNe64
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: mov [[RESULT:.*]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: jmp {{.*}}
; OPTM1-NEXT: call
; OPTM1: mov [[RESULT:.*]],0x1
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: cmp {{.*}}
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: mov [[RESULT:.*]],0x0
; OPTM1-NEXT: cmp [[RESULT]],0x0
; OPTM1-NEXT: jne {{.*}}
; OPTM1-NEXT: jmp {{.*}}
; OPTM1-NEXT: call

; ARM32-LABEL: icmpNe64
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: beq
; ARM32: bl {{.*}} <func>
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: beq
; ARM32: bl

; MIPS32-LABEL: icmpNe64
; MIPS32: xor [[T1:.*]],{{.*}},{{.*}}
; MIPS32: xor [[T2:.*]],{{.*}},{{.*}}
; MIPS32: or [[T3:.*]],[[T1]],[[T2]]
; MIPS32-O2: beqz [[T3]],{{.*}}
; MIPS32-OM1: sltu [[T4:.*]],zero,[[T3]]
; MIPS32-OM1: sw [[T4]],[[MEM:.*]]
; MIPS32-OM1: lb [[T5:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T5]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: xor [[T1:.*]],{{.*}},{{.*}}
; MIPS32: xor [[T2:.*]],{{.*}},{{.*}}
; MIPS32: or [[T3:.*]],[[T1]],[[T2]]
; MIPS32-O2: beqz [[T3]],{{.*}}
; MIPS32-OM1: sltu [[T4:.*]],zero,[[T3]]
; MIPS32-OM1: sw [[T4]],[[MEM:.*]]
; MIPS32-OM1: lb [[T5:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T5]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}

define internal void @icmpGt64(i64 %a, i64 %b, i64 %c, i64 %d) {
entry:
  %cmp = icmp ugt i64 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp sgt i64 %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: icmpGt64
; CHECK: ja
; CHECK: jb
; CHECK: jbe
; CHECK: call
; CHECK: jg
; CHECK: jl
; CHECK: jbe
; CHECK: call
;
; OPTM1-LABEL: icmpGt64
; OPTM1: ja
; OPTM1: jb
; OPTM1: ja
; OPTM1: call
; OPTM1: jg
; OPTM1: jl
; OPTM1: ja
; OPTM1: call

; ARM32-LABEL: icmpGt64
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: bls
; ARM32: bl
; ARM32: cmp
; ARM32: sbcs
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: bge
; ARM32: bl

; MIPS32-LABEL: icmpGt64
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-O2: sltu [[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32-O2: xori [[T3:.*]],[[T2]],0x1
; MIPS32-O2: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-O2: xori [[T5:.*]],[[T4]],0x1
; MIPS32-OM1: sltu [[T3:.*]],[[B_HI]],[[A_HI]]
; MIPS32-OM1: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-O2: slt [[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32-O2: xori [[T3:.*]],[[T2]],0x1
; MIPS32-O2: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-O2: xori [[T5:.*]],[[T4]],0x1
; MIPS32-OM1: slt [[T3:.*]],[[B_HI]],[[A_HI]]
; MIPS32-OM1: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}

define internal void @icmpGe64(i64 %a, i64 %b, i64 %c, i64 %d) {
entry:
  %cmp = icmp uge i64 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp sge i64 %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.end, %if.then2
  ret void
}
; CHECK-LABEL: icmpGe64
; CHECK: ja
; CHECK: jb
; CHECK: jb
; CHECK: call
; CHECK: jg
; CHECK: jl
; CHECK: jb
; CHECK: call
;
; OPTM1-LABEL: icmpGe64
; OPTM1: ja
; OPTM1: jb
; OPTM1: jae
; OPTM1: call
; OPTM1: jg
; OPTM1: jl
; OPTM1: jae
; OPTM1: call

; ARM32-LABEL: icmpGe64
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: bcc
; ARM32: bl
; ARM32: cmp
; ARM32: sbcs
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: blt
; ARM32: bl

; MIPS32-LABEL: icmpGe64
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-OM1: sltu [[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32-OM1: xori [[T3:.*]],[[T2]],0x1
; MIPS32-OM1: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-OM1: xori [[T5:.*]],[[T4]],0x1
; MIPS32-O2: sltu [[T3:.*]],[[A_HI]],[[B_HI]]
; MIPS32-O2: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-OM1: slt [[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32-OM1: xori [[T3:.*]],[[T2]],0x1
; MIPS32-OM1: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-OM1: xori [[T5:.*]],[[T4]],0x1
; MIPS32-O2: slt [[T3:.*]],[[A_HI]],[[B_HI]]
; MIPS32-O2: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}

define internal void @icmpLt64(i64 %a, i64 %b, i64 %c, i64 %d) {
entry:
  %cmp = icmp ult i64 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp slt i64 %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; CHECK-LABEL: icmpLt64
; CHECK: jb
; CHECK: ja
; CHECK: jae
; CHECK: call
; CHECK: jl
; CHECK: jg
; CHECK: jae
; CHECK: call
;
; OPTM1-LABEL: icmpLt64
; OPTM1: jb
; OPTM1: ja
; OPTM1: jb
; OPTM1: call
; OPTM1: jl
; OPTM1: jg
; OPTM1: jb
; OPTM1: call

; ARM32-LABEL: icmpLt64
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: bcs
; ARM32: bl
; ARM32: cmp
; ARM32: sbcs
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: bge
; ARM32: bl

; MIPS32-LABEL: icmpLt64
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-O2: sltu [[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32-O2: xori [[T3:.*]],[[T2]],0x1
; MIPS32-O2: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-O2: xori [[T5:.*]],[[T4]],0x1
; MIPS32-OM1: sltu [[T3:.*]],[[A_HI]],[[B_HI]]
; MIPS32-OM1: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-O2: slt [[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32-O2: xori [[T3:.*]],[[T2]],0x1
; MIPS32-O2: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-O2: xori [[T5:.*]],[[T4]],0x1
; MIPS32-OM1: slt [[T3:.*]],[[A_HI]],[[B_HI]]
; MIPS32-OM1: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}

define internal void @icmpLe64(i64 %a, i64 %b, i64 %c, i64 %d) {
entry:
  %cmp = icmp ule i64 %a, %b
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp sle i64 %c, %d
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.end, %if.then2
  ret void
}
; CHECK-LABEL: icmpLe64
; CHECK: jb
; CHECK: ja
; CHECK: ja
; CHECK: call
; CHECK: jl
; CHECK: jg
; CHECK: ja
; CHECK: call
;
; OPTM1-LABEL: icmpLe64
; OPTM1: jb
; OPTM1: ja
; OPTM1: jbe
; OPTM1: call
; OPTM1: jl
; OPTM1: jg
; OPTM1: jbe
; OPTM1: call

; ARM32-LABEL: icmpLe64
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: bhi
; ARM32: bl
; ARM32: cmp
; ARM32: sbcs
; ARM32-OM1: tst
; ARM32-OM1: bne
; ARM32-O2: blt
; ARM32: bl

; MIPS32-LABEL: icmpLe64
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-OM1: sltu [[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32-OM1: xori [[T3:.*]],[[T2]],0x1
; MIPS32-OM1: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-OM1: xori [[T5:.*]],[[T4]],0x1
; MIPS32-O2: sltu [[T3:.*]],[[B_HI]],[[A_HI]]
; MIPS32-O2: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: xor [[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32-OM1: slt [[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32-OM1: xori [[T3:.*]],[[T2]],0x1
; MIPS32-OM1: sltu [[T4:.*]],{{.*}},{{.*}}
; MIPS32-OM1: xori [[T5:.*]],[[T4]],0x1
; MIPS32-O2: slt [[T3:.*]],[[B_HI]],[[A_HI]]
; MIPS32-O2: sltu [[T5:.*]],{{.*}},{{.*}}
; MIPS32: movz [[T3]],[[T5]],[[T1]]
; MIPS32-O2: bnez [[T3]],{{.*}}
; MIPS32-OM1: sw [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb [[T6:.*]],[[MEM]]
; MIPS32-OM1: beqz [[T6]],{{.*}}
; MIPS32-OM1: b {{.*}}
; MIPS32: jal {{.*}}
; MIPS32-OM1: b {{.*}}

define internal i32 @icmpEq64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp eq i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpEq64Bool
; CHECK: jne
; CHECK: je
;
; OPTM1-LABEL: icmpEq64Bool
; OPTM1: jne
; OPTM1: je

; ARM32-LABEL: icmpEq64Bool
; ARM32: mov
; ARM32: moveq

; MIPS32-LABEL: icmpEq64Bool
; MIPS32: xor	[[T1:.*]],{{.*}},{{.*}}
; MIPS32: xor	[[T2:.*]],{{.*}},{{.*}}
; MIPS32: or	[[T3:.*]],[[T1]],[[T2]]
; MIPS32: sltiu	{{.*}},[[T3]],1

define internal i32 @icmpNe64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp ne i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpNe64Bool
; CHECK: jne
; CHECK: jne
;
; OPTM1-LABEL: icmpNe64Bool
; OPTM1: jne
; OPTM1: jne

; ARM32-LABEL: icmpNe64Bool
; ARM32: mov
; ARM32: movne

; MIPS32-LABEL: icmpNe64Bool
; MIPS32: xor   [[T1:.*]],{{.*}},{{.*}}
; MIPS32: xor   [[T2:.*]],{{.*}},{{.*}}
; MIPS32: or    [[T3:.*]],[[T1]],[[T2]]
; MIPS32: sltu {{.*}},zero,[[T3]]

define internal i32 @icmpSgt64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp sgt i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpSgt64Bool
; CHECK: cmp
; CHECK: jg
; CHECK: jl
; CHECK: cmp
; CHECK: ja
;
; OPTM1-LABEL: icmpSgt64Bool
; OPTM1: cmp
; OPTM1: jg
; OPTM1: jl
; OPTM1: cmp
; OPTM1: ja

; ARM32-LABEL: icmpSgt64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: sbcs
; ARM32: movlt

; MIPS32-LABEL: icmpSgt64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: slt	[[T2:.*]],{{.*}},{{.*}}
; MIPS32: sltu	[[T3:.*]],{{.*}},{{.*}}
; MIPS32: movz	[[T2]],[[T3]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T2]]
; MIPS32-OM1: sw	[[T2]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i32 @icmpUgt64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp ugt i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpUgt64Bool
; CHECK: cmp
; CHECK: ja
; CHECK: jb
; CHECK: cmp
; CHECK: ja
;
; OPTM1-LABEL: icmpUgt64Bool
; OPTM1: cmp
; OPTM1: ja
; OPTM1: jb
; OPTM1: cmp
; OPTM1: ja

; ARM32-LABEL: icmpUgt64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: cmpeq
; ARM32: movhi

; MIPS32-LABEL: icmpUgt64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: sltu	[[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32: sltu	[[T3:.*]],{{.*}},{{.*}}
; MIPS32: movz	[[T2]],[[T3]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T2]]
; MIPS32-OM1: sw	[[T2]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i32 @icmpSge64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp sge i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpSge64Bool
; CHECK: cmp
; CHECK: jg
; CHECK: jl
; CHECK: cmp
; CHECK: jae
;
; OPTM1-LABEL: icmpSge64Bool
; OPTM1: cmp
; OPTM1: jg
; OPTM1: jl
; OPTM1: cmp
; OPTM1: jae

; ARM32-LABEL: icmpSge64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: sbcs
; ARM32: movge

; MIPS32-LABEL: icmpSge64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: slt	[[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32: xori	[[T3:.*]],[[T2]],0x1
; MIPS32: sltu	[[T4:.*]],{{.*}},{{.*}}
; MIPS32: xori	[[T5:.*]],[[T4]],0x1
; MIPS32: movz	[[T6:.*]],[[T5]],[[T1]]
; MIPS32-O2: move       {{.*}},[[T3]]
; MIPS32-OM1: sw        [[T3]],[[MEM:.*]]
; MIPS32-OM1: lb        {{.*}},[[MEM]]

define internal i32 @icmpUge64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp uge i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpUge64Bool
; CHECK: cmp
; CHECK: ja
; CHECK: jb
; CHECK: cmp
; CHECK: jae
;
; OPTM1-LABEL: icmpUge64Bool
; OPTM1: cmp
; OPTM1: ja
; OPTM1: jb
; OPTM1: cmp
; OPTM1: jae

; ARM32-LABEL: icmpUge64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: cmpeq
; ARM32: movcs

; MIPS32-LABEL: icmpUge64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: sltu	[[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32: xori	[[T3:.*]],[[T2]],0x1
; MIPS32: sltu	[[T4:.*]],{{.*}},{{.*}}
; MIPS32: xori	[[T5:.*]],[[T4]],0x1
; MIPS32: movz	[[T6:.*]],[[T5]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T3]]
; MIPS32-OM1: sw	[[T3]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i32 @icmpSlt64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp slt i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpSlt64Bool
; CHECK: cmp
; CHECK: jl
; CHECK: jg
; CHECK: cmp
; CHECK: jb
;
; OPTM1-LABEL: icmpSlt64Bool
; OPTM1: cmp
; OPTM1: jl
; OPTM1: jg
; OPTM1: cmp
; OPTM1: jb

; ARM32-LABEL: icmpSlt64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: sbcs
; ARM32: movlt

; MIPS32-LABEL: icmpSlt64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: slt	[[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32: sltu	[[T3:.*]],{{.*}},{{.*}}
; MIPS32: movz	[[T2:.*]],[[T3]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T2]]
; MIPS32-OM1: sw	[[T2]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i32 @icmpUlt64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp ult i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpUlt64Bool
; CHECK: cmp
; CHECK: jb
; CHECK: ja
; CHECK: cmp
; CHECK: jb
;
; OPTM1-LABEL: icmpUlt64Bool
; OPTM1: cmp
; OPTM1: jb
; OPTM1: ja
; OPTM1: cmp
; OPTM1: jb

; ARM32-LABEL: icmpUlt64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: cmpeq
; ARM32: movcc

; MIPS32-LABEL: icmpUlt64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: sltu	[[T2:.*]],[[A_HI]],[[B_HI]]
; MIPS32: sltu	[[T3:.*]],{{.*}},{{.*}}
; MIPS32: movz	[[T2:.*]],[[T3]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T2]]
; MIPS32-OM1: sw	[[T2]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i32 @icmpSle64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp sle i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpSle64Bool
; CHECK: cmp
; CHECK: jl
; CHECK: jg
; CHECK: cmp
; CHECK: jbe
;
; OPTM1-LABEL: icmpSle64Bool
; OPTM1: cmp
; OPTM1: jl
; OPTM1: jg
; OPTM1: cmp
; OPTM1: jbe

; ARM32-LABEL: icmpSle64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: sbcs
; ARM32: movge

; MIPS32-LABEL: icmpSle64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: slt	[[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32: xori	[[T3:.*]],[[T2]],0x1
; MIPS32: sltu	[[T4:.*]],{{.*}},{{.*}}
; MIPS32: xori	[[T5:.*]],[[T4]],0x1
; MIPS32: movz	[[T6:.*]],[[T5]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T3]]
; MIPS32-OM1: sw	[[T3]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i32 @icmpUle64Bool(i64 %a, i64 %b) {
entry:
  %cmp = icmp ule i64 %a, %b
  %cmp.ret_ext = zext i1 %cmp to i32
  ret i32 %cmp.ret_ext
}
; CHECK-LABEL: icmpUle64Bool
; CHECK: cmp
; CHECK: jb
; CHECK: ja
; CHECK: cmp
; CHECK: jbe
;
; OPTM1-LABEL: icmpUle64Bool
; OPTM1: cmp
; OPTM1: jb
; OPTM1: ja
; OPTM1: cmp
; OPTM1: jbe

; ARM32-LABEL: icmpUle64Bool
; ARM32: mov
; ARM32: cmp
; ARM32: cmpeq
; ARM32: movls

; MIPS32-LABEL: icmpUle64Bool
; MIPS32: xor	[[T1:.*]],[[A_HI:.*]],[[B_HI:.*]]
; MIPS32: sltu	[[T2:.*]],[[B_HI]],[[A_HI]]
; MIPS32: xori	[[T3:.*]],[[T2]],0x1
; MIPS32: sltu	[[T4:.*]],{{.*}},{{.*}}
; MIPS32: xori	[[T5:.*]],[[T4]],0x1
; MIPS32: movz	[[T6:.*]],[[T5]],[[T1]]
; MIPS32-O2: move	{{.*}},[[T3]]
; MIPS32-OM1: sw	[[T3]],[[MEM:.*]]
; MIPS32-OM1: lb	{{.*}},[[MEM]]

define internal i64 @load64(i32 %a) {
entry:
  %__1 = inttoptr i32 %a to i64*
  %v0 = load i64, i64* %__1, align 1
  ret i64 %v0
}
; CHECK-LABEL: load64
; CHECK: mov e[[REGISTER:[a-z]+]],DWORD PTR [esp+0x4]
; CHECK-NEXT: mov {{.*}},DWORD PTR [e[[REGISTER]]]
; CHECK-NEXT: mov {{.*}},DWORD PTR [e[[REGISTER]]+0x4]
;
; OPTM1-LABEL: load64
; OPTM1: mov e{{..}},DWORD PTR [e{{..}}]
; OPTM1: mov e{{..}},DWORD PTR [e{{..}}+0x4]

; ARM32-LABEL: load64
; ARM32: ldr r{{.*}}, [r[[REG:.*]]]
; ARM32: ldr r{{.*}}, [r[[REG]], #4]

; MIPS32-LABEL: load64
; MIPS32-O2: 	lw	{{.*}},0([[REG:.*]])
; MIPS32-O2: 	lw	[[REG]],4([[REG]])

define internal void @store64(i32 %a, i64 %value) {
entry:
  %__2 = inttoptr i32 %a to i64*
  store i64 %value, i64* %__2, align 1
  ret void
}
; CHECK-LABEL: store64
; CHECK: mov e[[REGISTER:[a-z]+]],DWORD PTR [esp+0x4]
; CHECK: mov DWORD PTR [e[[REGISTER]]+0x4],
; CHECK: mov DWORD PTR [e[[REGISTER]]],
;
; OPTM1-LABEL: store64
; OPTM1: mov DWORD PTR [e[[REGISTER:[a-z]+]]+0x4],
; OPTM1: mov DWORD PTR [e[[REGISTER]]],

; ARM32-LABEL: store64
; ARM32: str r{{.*}}, [r[[REG:.*]], #4]
; ARM32: str r{{.*}}, [r[[REG]]]

; MIPS32-LABEL: store64
; MIPS32-O2: 	sw	{{.*}},4([[REG:.*]])
; MIPS32-O2: 	sw	{{.*}},0([[REG]])

define internal void @store64Const(i32 %a) {
entry:
  %__1 = inttoptr i32 %a to i64*
  store i64 -2401053092306725256, i64* %__1, align 1
  ret void
}
; CHECK-LABEL: store64Const
; CHECK: mov e[[REGISTER:[a-z]+]],DWORD PTR [esp+0x4]
; CHECK: mov DWORD PTR [e[[REGISTER]]+0x4],0xdeadbeef
; CHECK: mov DWORD PTR [e[[REGISTER]]],0x12345678
;
; OPTM1-LABEL: store64Const
; OPTM1: mov DWORD PTR [e[[REGISTER:[a-z]+]]+0x4],0xdeadbeef
; OPTM1: mov DWORD PTR [e[[REGISTER]]],0x12345678

; ARM32-LABEL: store64Const
; ARM32: movw [[REG1:.*]], #48879 ; 0xbeef
; ARM32: movt [[REG1:.*]], #57005 ; 0xdead
; ARM32: movw [[REG2:.*]], #22136 ; 0x5678
; ARM32: movt [[REG2:.*]], #4660  ; 0x1234
; ARM32: str [[REG1]], [r[[REG:.*]], #4]
; ARM32: str [[REG2]], [r[[REG]]]

; MIPS32-LABEL: store64Const
; MIPS32-O2: 	lui	[[REG1:.*]],0xdead
; MIPS32-O2: 	ori	[[REG1:.*]],[[REG1]],0xbeef
; MIPS32-O2: 	lui	[[REG2:.*]],0x1234
; MIPS32-O2: 	ori	[[REG2:.*]],[[REG2]],0x5678
; MIPS32-O2: 	sw	[[REG1]],4([[REG:.*]])
; MIPS32-O2: 	sw	[[REG2]],0([[REG]])

define internal i64 @select64VarVar(i64 %a, i64 %b) {
entry:
  %cmp = icmp ult i64 %a, %b
  %cond = select i1 %cmp, i64 %a, i64 %b
  ret i64 %cond
}
; CHECK-LABEL: select64VarVar
; CHECK: mov
; CHECK: mov
; CHECK: cmp
; CHECK: jb
; CHECK: ja
; CHECK: cmp
; CHECK: jb
; CHECK: mov
; CHECK: mov
;
; OPTM1-LABEL: select64VarVar
; OPTM1: cmp
; OPTM1: jb
; OPTM1: ja
; OPTM1: cmp
; OPTM1: jb
; OPTM1: cmp
; OPTM1: cmovne

; ARM32-LABEL: select64VarVar
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: movne
; ARM32-O2: movcc
; ARM32-OM1: movne
; ARM32-O2: movcc

; MIPS32-LABEL: select64VarVar
; MIPS32: movn
; MIPS32: movn

define internal i64 @select64VarConst(i64 %a, i64 %b) {
entry:
  %cmp = icmp ult i64 %a, %b
  %cond = select i1 %cmp, i64 %a, i64 -2401053092306725256
  ret i64 %cond
}
; CHECK-LABEL: select64VarConst
; CHECK: mov
; CHECK: mov
; CHECK: cmp
; CHECK: jb
; CHECK: ja
; CHECK: cmp
; CHECK: jb
; CHECK: mov
; CHECK: mov
;
; OPTM1-LABEL: select64VarConst
; OPTM1: cmp
; OPTM1: jb
; OPTM1: ja
; OPTM1: cmp
; OPTM1: jb
; OPTM1: cmp
; OPTM1: cmovne

; ARM32-LABEL: select64VarConst
; ARM32: mov
; ARM32: mov
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32-OM1: movne
; ARM32-O2: movcc
; ARM32-OM1: movne
; ARM32-O2: movcc
; ARM32-O2: mov
; ARM32-O2: mov

; MIPS32-LABEL: select64VarConst
; MIPS32: movn
; MIPS32: movn

define internal i64 @select64ConstVar(i64 %a, i64 %b) {
entry:
  %cmp = icmp ult i64 %a, %b
  %cond = select i1 %cmp, i64 -2401053092306725256, i64 %b
  ret i64 %cond
}
; CHECK-LABEL: select64ConstVar
; CHECK: mov
; CHECK: mov
; CHECK: cmp
; CHECK: jb
; CHECK: ja
; CHECK: cmp
; CHECK: jb
; CHECK: mov
; CHECK: mov
;
; OPTM1-LABEL: select64ConstVar
; OPTM1: cmp
; OPTM1: jb
; OPTM1: ja
; OPTM1: cmp
; OPTM1: jb
; OPTM1: cmp
; OPTM1: cmove

; ARM32-LABEL: select64ConstVar
; ARM32: cmp
; ARM32: cmpeq
; ARM32-OM1: tst
; ARM32: movw
; ARM32: movt
; ARM32-OM1: movne
; ARM32-O2: movcc
; ARM32: movw
; ARM32: movt
; ARM32-OM1: movne
; ARM32-O2: movcc

; MIPS32-LABEL: select64ConstVar
; MIPS32: movn
; MIPS32: movn

define internal void @icmpEq64Imm() {
entry:
  %cmp = icmp eq i64 123, 234
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp eq i64 345, 456
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpEq64Imm
; CHECK-NOT: cmp 0x{{[0-9a-f]+}},
; OPTM1-LABEL: icmpEq64Imm
; OPTM1-LABEL-NOT: cmp 0x{{[0-9a-f]+}},
; ARM32-LABEL: icmpEq64Imm
; ARM32-NOT: cmp #{{[0-9a-f]+}},

define internal void @icmpLt64Imm() {
entry:
  %cmp = icmp ult i64 123, 234
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  call void @func()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %cmp1 = icmp slt i64 345, 456
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  call void @func()
  br label %if.end3

if.end3:                                          ; preds = %if.then2, %if.end
  ret void
}
; The following checks are not strictly necessary since one of the RUN
; lines actually runs the output through the assembler.
; CHECK-LABEL: icmpLt64Imm
; CHECK-NOT: cmp 0x{{[0-9a-f]+}},
; OPTM1-LABEL: icmpLt64Imm
; OPTM1-NOT: cmp 0x{{[0-9a-f]+}},
; ARM32-LABEL: icmpLt64Imm
; ARM32-NOT: cmp #{{[0-9a-f]+}},

define internal i64 @phi64Imm(i32 %x, i64 %y, i64 %z) {
entry:
  %cond = icmp eq i32 %x, 88
  br i1 %cond, label %branch1, label %branch2
branch1:
  %tmp = add i64 %y, %z
  br label %branch2

branch2:
  %merge = phi i64 [ %tmp, %branch1 ], [ 20014547621496, %entry ]
  ret i64 %merge
}
; CHECK-LABEL: phi64Imm
; CHECK: mov {{.*}},0x5678
; CHECK: mov {{.*}},0x1234
; OPTM1-LABEL: phi64Imm
; OPTM1: mov {{.*}},0x5678
; OPTM1: mov {{.*}},0x1234
; ARM32-LABEL: phi64Imm
; ARM32: movw {{.*}}, #22136 ; 0x5678
; ARM32: movw {{.*}}, #4660  ; 0x1234

define internal i64 @phi64Undef(i32 %x, i64 %y, i64 %z) {
entry:
  %cond = icmp eq i32 %x, 88
  br i1 %cond, label %branch1, label %branch2
branch1:
  %tmp = add i64 %y, %z
  br label %branch2

branch2:
  %merge = phi i64 [ %tmp, %branch1 ], [ undef, %entry ]
  ret i64 %merge
}

; CHECK-LABEL: phi64Undef
; CHECK: mov {{.*}},0x0
; CHECK: mov {{.*}},0x0
; OPTM1-LABEL: phi64Undef
; OPTM1: mov {{.*}},0x0
; OPTM1: mov {{.*}},0x0
; ARM32-LABEL: phi64Undef
; ARM32: mov {{.*}} #0
; ARM32: mov {{.*}} #0

define internal i32 @addOneToUpperAfterShift(i64 %value) {
  %a = add i64 %value, 1
  %s = lshr i64 %a, 40
  %t = trunc i64 %s to i32
  %r = add i32 %t, 1
  ret i32 %r
; ARM32-LABEL: addOneToUpperAfterShift
; ARM32: adds
; ARM32: adc
; ARM32: lsr
; ARM32: add
}

define internal i32 @subOneToUpperAfterShift(i64 %value) {
  %a = sub i64 %value, 1
  %s = lshr i64 %a, 40
  %t = trunc i64 %s to i32
  %r = sub i32 %t, 1
  ret i32 %r
; ARM32-LABEL: subOneToUpperAfterShift
; ARM32: subs
; ARM32: sbc
; ARM32: lsr
; ARM32: sub
}
