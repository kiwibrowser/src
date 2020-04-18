; Test of global variable reordering.

; REQUIRES: allow_dump

; Test x8632 asm output
; RUN: %if --need=target_X8632 --command %p2i --filetype=asm --target x8632 \
; RUN:     -i %s --assemble --disassemble --dis-flags=-rD \
; RUN:      --args -sz-seed=1 -reorder-global-variables -O2 \
; RUN:     | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=asm --target x8632 \
; RUN:     -i %s --assemble --disassemble --dis-flags=-rD \
; RUN:     --args -sz-seed=1 -reorder-global-variables -Om1 \
; RUN:     | %if --need=target_X8632 --command FileCheck %s

; Test x8632 elf output
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --target x8632 \
; RUN:     -i %s --disassemble --dis-flags=-rD \
; RUN:     --args -sz-seed=1 -reorder-global-variables -O2 \
; RUN:     | %if --need=target_X8632 --command FileCheck %s
; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --target x8632 \
; RUN:     -i %s --disassemble --dis-flags=-rD \
; RUN:     --args -sz-seed=1 -reorder-global-variables -Om1 \
; RUN:     | %if --need=target_X8632 --command  FileCheck %s

; Test arm output
; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj --target arm32 \
; RUN:     -i %s --disassemble --dis-flags=-rD \
; RUN:     --args -sz-seed=1 -reorder-global-variables \
; RUN:     -O2 \
; RUN:     | %if --need=target_ARM32 --command FileCheck %s
; RUN: %if --need=target_ARM32 --command %p2i --filetype=obj --target arm32 \
; RUN:     -i %s --disassemble --dis-flags=-rD \
; RUN:     --args -sz-seed=1 -reorder-global-variables \
; RUN:     -Om1 \
; RUN:     | %if --need=target_ARM32 --command FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:     --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:     mips32 -i %s --dis-flags=-rD --args -O2 -sz-seed=1 \
; RUN:     -reorder-global-variables \
; RUN:     | %if --need=target_MIPS32 --need=allow_dump --command FileCheck %s

@PrimitiveInit = internal global [4 x i8] c"\1B\00\00\00", align 4

@PrimitiveInitConst = internal constant [4 x i8] c"\0D\00\00\00", align 4

@ArrayInit = internal global [20 x i8] c"\0A\00\00\00\14\00\00\00\1E\00\00\00(\00\00\002\00\00\00", align 4

@ArrayInitPartial = internal global [40 x i8] c"<\00\00\00F\00\00\00P\00\00\00Z\00\00\00d\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00", align 4

@PrimitiveInitStatic = internal global [4 x i8] zeroinitializer, align 4

@PrimitiveUninit = internal global [4 x i8] zeroinitializer, align 4

@ArrayUninit = internal global [20 x i8] zeroinitializer, align 4

@ArrayUninitConstDouble = internal constant [200 x i8] zeroinitializer, align 8

@ArrayUninitConstInt = internal constant [20 x i8] zeroinitializer, align 4

; Make sure the shuffled order is correct.

; CHECK-LABEL: ArrayInit
; CHECK-LABEL: PrimitiveInit
; CHECK-LABEL: ArrayInitPartial
; CHECK-LABEL: PrimitiveUninit
; CHECK-LABEL: ArrayUninit
; CHECK-LABEL: PrimitiveInitStatic
; CHECK-LABEL: ArrayUninitConstDouble
; CHECK-LABEL: ArrayUninitConstInt
; CHECK-LABEL: PrimitiveInitConst
