; Simple test of the select instruction.  The CHECK lines are only
; checking for basic instruction patterns that should be present
; regardless of the optimization level, so there are no special OPTM1
; match lines.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix ARM32-O2 %s
; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix ARM32-OM1 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

define internal void @testSelect(i32 %a, i32 %b) {
entry:
  %cmp = icmp slt i32 %a, %b
  %cond = select i1 %cmp, i32 %a, i32 %b
  tail call void @useInt(i32 %cond)
  %cmp1 = icmp sgt i32 %a, %b
  %cond2 = select i1 %cmp1, i32 10, i32 20
  tail call void @useInt(i32 %cond2)
  ; Create "fake" uses of %cmp and %cmp1 to prevent O2 bool folding.
  %d1 = zext i1 %cmp to i32
  call void @useInt(i32 %d1)
  %d2 = zext i1 %cmp1 to i32
  call void @useInt(i32 %d2)
  ret void
}

declare void @useInt(i32 %x)

; CHECK-LABEL: testSelect
; CHECK:      cmp
; CHECK:      cmp
; CHECK:      call {{.*}} R_{{.*}} useInt
; CHECK:      cmp
; CHECK:      cmp
; CHECK:      call {{.*}} R_{{.*}} useInt
; CHECK:      ret
; ARM32-LABEL: testSelect
; ARM32: cmp
; ARM32: bl {{.*}} useInt
; ARM32-Om1: mov {{.*}}, #20
; ARM32-O2: mov [[REG:r[0-9]+]], #20
; ARM32: tst
; ARM32-Om1: movne {{.*}}, #10
; ARM32-O2: movne [[REG]], #10
; ARM32: bl {{.*}} useInt
; ARM32: bl {{.*}} useInt
; ARM32: bl {{.*}} useInt
; ARM32: bx lr
; MIPS32-LABEL: testSelect
; MIPS32: slt {{.*}}
; MIPS32: movn {{.*}}

; Check for valid addressing mode in the cmp instruction when the
; operand is an immediate.
define internal i32 @testSelectImm32(i32 %a, i32 %b) {
entry:
  %cond = select i1 false, i32 %a, i32 %b
  ret i32 %cond
}
; CHECK-LABEL: testSelectImm32
; CHECK-NOT: cmp 0x{{[0-9a-f]+}},
; ARM32-LABEL: testSelectImm32
; ARM32-NOT: cmp #{{.*}},
; MIPS32-LABEL: testSelectImm32
; MIPS32: movn {{.*}}

; Check for valid addressing mode in the cmp instruction when the
; operand is an immediate.  There is a different x86-32 lowering
; sequence for 64-bit operands.
define internal i64 @testSelectImm64(i64 %a, i64 %b) {
entry:
  %cond = select i1 true, i64 %a, i64 %b
  ret i64 %cond
}
; CHECK-LABEL: testSelectImm64
; CHECK-NOT: cmp 0x{{[0-9a-f]+}},
; ARM32-LABEL: testSelectImm64
; ARM32-NOT: cmp #{{.*}},
; MIPS32-LABEL: testSelectImm64
; MIPS32: movn
; MIPS32: movn
