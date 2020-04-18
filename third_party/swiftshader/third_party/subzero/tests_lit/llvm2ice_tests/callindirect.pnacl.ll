; Test of multiple indirect calls to the same target.  Each call
; should be to the same operand, whether it's in a register or on the
; stack.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 \
; RUN:   | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=allow_dump --need=target_X8632 --command %p2i --filetype=asm \
; RUN:     --assemble --disassemble -i %s --args -O2 \
; RUN:   | %if --need=allow_dump --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 \
; RUN:   | %if --need=target_X8632 --command FileCheck --check-prefix=OPTM1 %s

; RUN: %if --need=target_X8664 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8664 -i %s --args -O2 \
; RUN:   | %if --need=target_X8664 --command FileCheck --check-prefix X8664 %s
; RUN: %if --need=allow_dump --need=target_X8664 --command %p2i --filetype=asm \
; RUN:     --assemble --disassemble --target x8664 -i %s --args -O2 \
; RUN:   | %if --need=allow_dump --need=target_X8664 \
; RUN:     --command FileCheck --check-prefix=X8664 %s
; RUN: %if --need=target_X8664 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8664 -i %s --args -Om1 \
; RUN:   | %if --need=target_X8664 \
; RUN:     --command FileCheck --check-prefix=X8664-OPTM1 %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -Om1 \
; RUN:   | %if --need=target_ARM32_dump \
; RUN:   --command FileCheck --check-prefix ARM32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

@__init_array_start = internal constant [0 x i8] zeroinitializer, align 4
@__fini_array_start = internal constant [0 x i8] zeroinitializer, align 4
@__tls_template_start = internal constant [0 x i8] zeroinitializer, align 8
@__tls_template_alignment = internal constant [4 x i8] c"\01\00\00\00", align 4

define internal void @CallIndirect(i32 %f) {
entry:
  %__1 = inttoptr i32 %f to void ()*
  call void %__1()
  call void %__1()
  call void %__1()
  call void %__1()
  call void %__1()
  call void %__1()
  ret void
}
; CHECK-LABEL: CallIndirect
; Use the first call as a barrier in case the register allocator decides to use
; a scratch register for it but a common preserved register for the rest.
; CHECK: call
; CHECK: call [[REGISTER:[a-z]+]]
; CHECK: call [[REGISTER]]
; CHECK: call [[REGISTER]]
; CHECK: call [[REGISTER]]
; CHECK: call [[REGISTER]]
;
; OPTM1-LABEL: CallIndirect
; OPTM1: call [[TARGET:.+]]
; OPTM1: call [[TARGET]]
; OPTM1: call [[TARGET]]
; OPTM1: call [[TARGET]]
; OPTM1: call [[TARGET]]
;
; X8664-LABEL: CallIndirect
; Use the first call as a barrier so we skip the movs in the function prolog.
; X8664: call r{{..}}
; X8664: mov e[[REG:..]],
; X8664-NEXT: call r[[REG]]
; X8664: mov e[[REG:..]],
; X8664-NEXT: call r[[REG]]
; X8664: mov e[[REG:..]],
; X8664-NEXT: call r[[REG]]
; X8664: call r{{..}}
;
; X8664-OPTM1-LABEL: CallIndirect
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
;
; ARM32-LABEL: CallIndirect
; ARM32: blx [[REGISTER:r.*]]
; ARM32: blx [[REGISTER]]
; ARM32: blx [[REGISTER]]
; ARM32: blx [[REGISTER]]
; ARM32: blx [[REGISTER]]

; MIPS32-LABEL: CallIndirect
; MIPS32: jalr	[[REGISTER:.*]]
; MIPS32: jalr	[[REGISTER]]
; MIPS32: jalr	[[REGISTER]]
; MIPS32: jalr	[[REGISTER]]

@fp_v = internal global [4 x i8] zeroinitializer, align 4

define internal void @CallIndirectGlobal() {
entry:
  %fp_ptr_i32 = bitcast [4 x i8]* @fp_v to i32*
  %fp_ptr = load i32, i32* %fp_ptr_i32, align 1
  %fp = inttoptr i32 %fp_ptr to void ()*
  call void %fp()
  call void %fp()
  call void %fp()
  call void %fp()
  ret void
}
; CHECK-LABEL: CallIndirectGlobal
; Allow the first call to be to a different register because of simple
; availability optimization.
; CHECK: call
; CHECK: call [[REGISTER:[a-z]+]]
; CHECK: call [[REGISTER]]
; CHECK: call [[REGISTER]]
;
; OPTM1-LABEL: CallIndirectGlobal
; OPTM1: call [[TARGET:.+]]
; OPTM1: call [[TARGET]]
; OPTM1: call [[TARGET]]
; OPTM1: call [[TARGET]]
;
; X8664-LABEL: CallIndirectGlobal
; X8664: call r[[REG]]
; X8664: mov e[[REG:..]]
; X8664-NEXT: call r[[REG]]
; X8664: mov e[[REG:..]]
; X8664-NEXT: call r[[REG]]
; X8664: call r{{..}}
;
; X8664-OPTM1-LABEL: CallIndirectGlobal
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
; X8664-OPTM1: mov e[[REG:..]],DWORD PTR
; X8664-OPTM1: call r[[REG]]
;
; ARM32-LABEL: CallIndirectGlobal
; ARM32: blx {{r.*}}
; ARM32: blx [[REGISTER:r[0-9]*]]
; ARM32: blx [[REGISTER]]
; ARM32: blx [[REGISTER]]

; MIPS32-LABEL: CallIndirectGlobal
; MIPS32: jalr	[[REGISTER:.*]]
; MIPS32: jalr	[[REGISTER]]
; MIPS32: jalr	[[REGISTER]]
; MIPS32: jalr	[[REGISTER]]

; Calling an absolute address is used for non-IRT PNaCl pexes to directly
; access syscall trampolines. This is not really an indirect call, but
; there is a cast from int to pointer first.
define internal void @CallConst() {
entry:
  %__1 = inttoptr i32 66496 to void ()*
  call void %__1()
  call void %__1()
  call void %__1()
  ret void
}

; CHECK-LABEL: CallConst
; CHECK: e8 bc 03 01 00 call {{[0-9a-f]+}} {{.*}} R_386_PC32 *ABS*
; CHECK: e8 bc 03 01 00 call {{[0-9a-f]+}} {{.*}} R_386_PC32 *ABS*
; CHECK: e8 bc 03 01 00 call {{[0-9a-f]+}} {{.*}} R_386_PC32 *ABS*
;
; OPTM1-LABEL: CallConst
; OPTM1: e8 bc 03 01 00 call {{[0-9a-f]+}} {{.*}} R_386_PC32 *ABS*
; OPTM1: e8 bc 03 01 00 call {{[0-9a-f]+}} {{.*}} R_386_PC32 *ABS*
; OPTM1: e8 bc 03 01 00 call {{[0-9a-f]+}} {{.*}} R_386_PC32 *ABS*
;
; X8664-LABEL: CallConst
; TODO(jpp): fix absolute call emission.
; These are broken: the emitted code should be
;    e8 00 00 00 00 call {{.*}} *ABS*+0x103bc
;
; X8664-OPTM1-LABEL: CallConst
; TODO(jpp): fix absolute call emission.
; These are broken: the emitted code should be
;    e8 00 00 00 00 call {{.*}} *ABS*+0x103bc
;
; ARM32-LABEL: CallConst
; ARM32: movw [[REGISTER:r.*]], #960
; ARM32: movt [[REGISTER]], #1
; ARM32: blx [[REGISTER]]
; The legalization of the constant could be shared, but it isn't.
; ARM32: movw [[REGISTER:r.*]], #960
; ARM32: blx [[REGISTER]]
; ARM32: blx [[REGISTER]]
