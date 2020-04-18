; Trivial smoke test of basic block reordering. Different random seeds should
; generate different basic block layout.
; REQUIRES: allow_dump

; RUN: %p2i -i %s --filetype=asm --args -O2 -sz-seed=1 \
; RUN: -reorder-basic-blocks -threads=0 \
; RUN: | FileCheck %s --check-prefix=SEED1
; RUN: %p2i -i %s --filetype=asm --args -O2 -sz-seed=2 \
; RUN: -reorder-basic-blocks -threads=0 \
; RUN: | FileCheck %s --check-prefix=SEED2

define internal void @basic_block_reordering(i32 %foo, i32 %bar) {
entry:
  %r1 = icmp eq i32 %foo, %bar
  br i1 %r1, label %BB1, label %BB2
BB1:
  %r2 = icmp sgt i32 %foo, %bar
  br i1 %r2, label %BB3, label %BB4
BB2:
  %r3 = icmp slt i32 %foo, %bar
  br i1 %r3, label %BB3, label %BB4
BB3:
  ret void
BB4:
  ret void


; SEED1-LABEL: basic_block_reordering:
; SEED1: .Lbasic_block_reordering$entry:
; SEED1: .Lbasic_block_reordering$BB1:
; SEED1: .Lbasic_block_reordering$BB2:
; SEED1: .Lbasic_block_reordering$BB4:
; SEED1: .Lbasic_block_reordering$BB3:

; SEED2-LABEL: basic_block_reordering:
; SEED2: .Lbasic_block_reordering$entry:
; SEED2: .Lbasic_block_reordering$BB2:
; SEED2: .Lbasic_block_reordering$BB1:
; SEED2: .Lbasic_block_reordering$BB4:
; SEED2: .Lbasic_block_reordering$BB3
}
