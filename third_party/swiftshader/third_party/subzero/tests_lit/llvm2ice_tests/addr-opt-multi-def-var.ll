; This is distilled from a real function that led to a bug in the
; address mode optimization code.  It followed assignment chains
; through non-SSA temporaries created from Phi instruction lowering.
;
; This test depends to some degree on the stability of "--verbose
; addropt" output format.

; REQUIRES: target_X8632
; REQUIRES: allow_dump
; RUN: %p2i -i %s --args -O2 --verbose addropt \
; RUN:   -allow-externally-defined-symbols | FileCheck %s

declare i32 @_calloc_r(i32, i32, i32)

define internal i32 @_Balloc(i32 %ptr, i32 %k) {
entry:
  %gep = add i32 %ptr, 76
  %gep.asptr = inttoptr i32 %gep to i32*
  %0 = load i32, i32* %gep.asptr, align 1
  %cmp = icmp eq i32 %0, 0
  br i1 %cmp, label %if.then, label %if.end5

if.then:                                          ; preds = %entry
  %call = tail call i32 @_calloc_r(i32 %ptr, i32 4, i32 33)
  %gep.asptr2 = inttoptr i32 %gep to i32*
  store i32 %call, i32* %gep.asptr2, align 1
  %cmp3 = icmp eq i32 %call, 0
  br i1 %cmp3, label %return, label %if.end5

if.end5:                                          ; preds = %if.then, %entry
  %1 = phi i32 [ %call, %if.then ], [ %0, %entry ]
  %gep_array = mul i32 %k, 4
  %gep2 = add i32 %1, %gep_array
  %gep2.asptr = inttoptr i32 %gep2 to i32*
  %2 = load i32, i32* %gep2.asptr, align 1
; The above load instruction is a good target for address mode
; optimization.  Correct analysis would lead to dump output like:
;   Starting computeAddressOpt for instruction:
;     [ 15]  %__13 = load i32, i32* %gep2.asptr, align 1
;   Instruction: [ 14]  %gep2.asptr = i32 %gep2
;     results in Base=%gep2, Index=<null>, Shift=0, Offset=0
;   Instruction: [ 13]  %gep2 = add i32 %__9, %gep_array
;     results in Base=%__9, Index=%gep_array, Shift=0, Offset=0
;   Instruction: [ 18]  %__9 = i32 %__9_phi
;     results in Base=%__9_phi, Index=%gep_array, Shift=0, Offset=0
;   Instruction: [ 12]  %gep_array = mul i32 %k, 4
;     results in Base=%__9_phi, Index=%k, Shift=2, Offset=0
;
; Incorrect, overly-aggressive analysis would lead to output like:
;   Starting computeAddressOpt for instruction:
;     [ 15]  %__13 = load i32, i32* %gep2.asptr, align 1
;   Instruction: [ 14]  %gep2.asptr = i32 %gep2
;     results in Base=%gep2, Index=<null>, Shift=0, Offset=0
;   Instruction: [ 13]  %gep2 = add i32 %__9, %gep_array
;     results in Base=%__9, Index=%gep_array, Shift=0, Offset=0
;   Instruction: [ 18]  %__9 = i32 %__9_phi
;     results in Base=%__9_phi, Index=%gep_array, Shift=0, Offset=0
;   Instruction: [ 19]  %__9_phi = i32 %__4
;     results in Base=%__4, Index=%gep_array, Shift=0, Offset=0
;   Instruction: [ 12]  %gep_array = mul i32 %k, 4
;     results in Base=%__4, Index=%k, Shift=2, Offset=0
;
; CHECK-NOT: results in Base=%__4,
;
  ret i32 %2

return:                                           ; preds = %if.then
  ret i32 0
}
