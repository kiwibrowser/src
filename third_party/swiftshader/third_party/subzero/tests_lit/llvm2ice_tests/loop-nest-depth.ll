; Test the the loop nest depth is correctly calculated for basic blocks.

; REQUIRES: allow_dump

; Single threaded so that the dumps used for checking happen in order.
; RUN: %p2i --filetype=obj --disassemble -i %s --args -O2 --verbose=loop \
; RUN:     -log=%t --threads=0 && FileCheck %s < %t

define internal void @test_single_loop(i32 %a32) {
entry:
  %a = trunc i32 %a32 to i1
  br label %loop0

loop0:                               ; <-+
  br label %loop1                    ;   |
loop1:                               ;   |
  br i1 %a, label %loop0, label %out ; --+

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_single_loop_with_continue(i32 %a32, i32 %b32) {
entry:
  %a = trunc i32 %a32 to i1
  %b = trunc i32 %b32 to i1
  br label %loop0

loop0:                                 ; <-+
  br label %loop1                      ;   |
loop1:                                 ;   |
  br i1 %a, label %loop0, label %loop2 ; --+
loop2:                                 ;   |
  br i1 %b, label %loop0, label %out   ; --+

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop2:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_multiple_exits(i32 %a32, i32 %b32) {
entry:
  %a = trunc i32 %a32 to i1
  %b = trunc i32 %b32 to i1
  br label %loop0

loop0:                               ; <-+
  br label %loop1                    ;   |
loop1:                               ;   |
  br i1 %a, label %loop2, label %out ; --+-+
loop2:                               ;   | |
  br i1 %b, label %loop0, label %out ; --+ |
                                     ;     |
out:                                 ; <---+
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop2:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_two_nested_loops(i32 %a32, i32 %b32) {
entry:
  %a = trunc i32 %a32 to i1
  %b = trunc i32 %b32 to i1
  br label %loop0_0

loop0_0:                                   ; <---+
  br label %loop1_0                        ;     |
loop1_0:                                   ; <-+ |
  br label %loop1_1                        ;   | |
loop1_1:                                   ;   | |
  br i1 %a, label %loop1_0, label %loop0_1 ; --+ |
loop0_1:                                   ;     |
  br i1 %b, label %loop0_0, label %out     ; ----+

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0_0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1_0:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop1_1:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop0_1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_two_nested_loops_with_continue(i32 %a32, i32 %b32,
                                                          i32 %c32) {
entry:
  %a = trunc i32 %a32 to i1
  %b = trunc i32 %b32 to i1
  %c = trunc i32 %c32 to i1
  br label %loop0_0

loop0_0:                                   ; <---+
  br label %loop1_0                        ;     |
loop1_0:                                   ; <-+ |
  br label %loop1_1                        ;   | |
loop1_1:                                   ;   | |
  br i1 %a, label %loop1_0, label %loop1_2 ; --+ |
loop1_2:                                   ;   | |
  br i1 %a, label %loop1_0, label %loop0_1 ; --+ |
loop0_1:                                   ;     |
  br i1 %b, label %loop0_0, label %out     ; ----+

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0_0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1_0:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop1_1:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop1_2:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop0_1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_multiple_nested_loops(i32 %a32, i32 %b32) {
entry:
  %a = trunc i32 %a32 to i1
  %b = trunc i32 %b32 to i1
  br label %loop0_0

loop0_0:                                   ; <---+
  br label %loop1_0                        ;     |
loop1_0:                                   ; <-+ |
  br label %loop1_1                        ;   | |
loop1_1:                                   ;   | |
  br i1 %a, label %loop1_0, label %loop0_1 ; --+ |
loop0_1:                                   ;     |
  br label %loop2_0                        ;     |
loop2_0:                                   ; <-+ |
  br label %loop2_1                        ;   | |
loop2_1:                                   ;   | |
  br i1 %a, label %loop2_0, label %loop0_2 ; --+ |
loop0_2:                                   ;     |
  br i1 %b, label %loop0_0, label %out     ; ----+

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0_0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1_0:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop1_1:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop0_1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop2_0:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop2_1:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop0_2:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_three_nested_loops(i32 %a32, i32 %b32, i32 %c32) {
entry:
  %a = trunc i32 %a32 to i1
  %b = trunc i32 %b32 to i1
  %c = trunc i32 %c32 to i1
  br label %loop0_0

loop0_0:                                   ; <-----+
  br label %loop1_0                        ;       |
loop1_0:                                   ; <---+ |
  br label %loop2_0                        ;     | |
loop2_0:                                   ; <-+ | |
  br label %loop2_1                        ;   | | |
loop2_1:                                   ;   | | |
  br i1 %a, label %loop2_0, label %loop1_1 ; --+ | |
loop1_1:                                   ;     | |
  br i1 %b, label %loop1_0, label %loop0_1 ; ----+ |
loop0_1:                                   ;       |
  br i1 %c, label %loop0_0, label %out     ; ------+

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: loop0_0:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: loop1_0:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop2_0:
; CHECK-NEXT: LoopNestDepth = 3
; CHECK-NEXT: loop2_1:
; CHECK-NEXT: LoopNestDepth = 3
; CHECK-NEXT: loop1_1:
; CHECK-NEXT: LoopNestDepth = 2
; CHECK-NEXT: loop0_1:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_diamond(i32 %a32) {
entry:
  %a = trunc i32 %a32 to i1
  br i1 %a, label %left, label %right

left:
  br label %out

right:
  br label %out

out:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: left:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: right:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: out:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW

define internal void @test_single_block_loop(i32 %count) {
entry:
  br label %body
body:
;  %i = phi i32 [ 0, %entry ], [ %inc, %body ]
; A normal loop would have a phi instruction like above for the induction
; variable, but that may introduce new basic blocks due to phi edge splitting,
; so we use an alternative definition for %i to make the test more clear.
  %i = add i32 %count, 1
  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, %count
  br i1 %cmp, label %body, label %exit
exit:
  ret void
}

; CHECK-LABEL: After loop analysis
; CHECK-NEXT: entry:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-NEXT: body:
; CHECK-NEXT: LoopNestDepth = 1
; CHECK-NEXT: exit:
; CHECK-NEXT: LoopNestDepth = 0
; CHECK-LABEL: Before RMW
