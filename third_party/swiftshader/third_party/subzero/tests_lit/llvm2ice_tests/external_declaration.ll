; Tests that any symbols with special names have specially treated linkage.

; RUN: %if --need=allow_dump --command %p2i -i %s --filetype=asm \
; RUN:   --args -nonsfi=0 \
; RUN:   | %if --need=allow_dump --command FileCheck %s

; Verify that "__pnacl_pso_root", a specially named symbol, is made global, but
; other global variables are not.
@__pnacl_pso_root = constant [4 x i8] c"abcd";
@__pnacl_pso_not_root = constant [4 x i8] c"efgh";

; CHECK: .globl __pnacl_pso_root
; CHECK-NOT: .globl __pnacl_pso_not_root
