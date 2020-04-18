; Tests the integrated assembler for instructions with multiple
; relocations.

; RUN: %if --need=allow_dump --command %p2i -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --command FileCheck %s

; char global_char;
; char *p_global_char;
; void dummy();
; void store_immediate_to_global() { p_global_char = &global_char; }
; void add_in_place() { p_global_char += (int)&global_char; }
; void cmp_global_immediate() { if (p_global_char == &global_char) dummy(); }

@global_char = internal global [1 x i8] zeroinitializer, align 1
@p_global_char = internal global [4 x i8] zeroinitializer, align 4
declare void @dummy()

define internal void @store_immediate_to_global() {
entry:
  %p_global_char.bc = bitcast [4 x i8]* @p_global_char to i32*
  %expanded1 = ptrtoint [1 x i8]* @global_char to i32
  store i32 %expanded1, i32* %p_global_char.bc, align 1
  ret void
}
; CHECK-LABEL: store_immediate_to_global
; CHECK: .long p_global_char
; CHECK: .long global_char

; Also exercises the RMW add operation.
define internal void @add_in_place() {
entry:
  %p_global_char.bc = bitcast [4 x i8]* @p_global_char to i32*
  %0 = load i32, i32* %p_global_char.bc, align 1
  %expanded1 = ptrtoint [1 x i8]* @global_char to i32
  %gep = add i32 %0, %expanded1
  %p_global_char.bc3 = bitcast [4 x i8]* @p_global_char to i32*
  store i32 %gep, i32* %p_global_char.bc3, align 1
  ret void
}
; CHECK-LABEL: add_in_place
; CHECK: .long p_global_char
; CHECK-NEXT: .long global_char

define internal void @cmp_global_immediate() {
entry:
  %p_global_char.bc = bitcast [4 x i8]* @p_global_char to i32*
  %0 = load i32, i32* %p_global_char.bc, align 1
  %expanded1 = ptrtoint [1 x i8]* @global_char to i32
  %cmp = icmp eq i32 %0, %expanded1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  tail call void @dummy()
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}
; CHECK-LABEL: cmp_global_immediate
; CHECK: .long p_global_char
; CHECK: .long global_char
; CHECK: .long dummy
