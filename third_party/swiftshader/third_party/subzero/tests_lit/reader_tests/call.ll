; Test handling of call instructions.

; RUN: %p2i -i %s --insts --args -allow-externally-defined-symbols \
; RUN: | FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing \
; RUN:        -allow-externally-defined-symbols | \
; RUN:   FileCheck --check-prefix=NOIR %s

define internal i32 @fib(i32 %n) {
entry:
  %cmp = icmp slt i32 %n, 2
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
  %sub = add i32 %n, -1
  %call = tail call i32 @fib(i32 %sub)
  %sub1 = add i32 %n, -2
  %call2 = tail call i32 @fib(i32 %sub1)
  %add = add i32 %call2, %call
  ret i32 %add

return:                                           ; preds = %entry
  ret i32 %n
}

; CHECK:      define internal i32 @fib(i32 %n) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %cmp = icmp slt i32 %n, 2
; CHECK-NEXT:   br i1 %cmp, label %return, label %if.end
; CHECK-NEXT: if.end:
; CHECK-NEXT:   %sub = add i32 %n, -1
; CHECK-NEXT:   %call = call i32 @fib(i32 %sub)
; CHECK-NEXT:   %sub1 = add i32 %n, -2
; CHECK-NEXT:   %call2 = call i32 @fib(i32 %sub1)
; CHECK-NEXT:   %add = add i32 %call2, %call
; CHECK-NEXT:   ret i32 %add
; CHECK-NEXT: return:
; CHECK-NEXT:   ret i32 %n
; CHECK-NEXT: }

define internal i32 @fact(i32 %n) {
entry:
  %cmp = icmp slt i32 %n, 2
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
  %sub = add i32 %n, -1
  %call = tail call i32 @fact(i32 %sub)
  %mul = mul i32 %call, %n
  ret i32 %mul

return:                                           ; preds = %entry
  ret i32 %n
}

; CHECK-NEXT: define internal i32 @fact(i32 %n) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %cmp = icmp slt i32 %n, 2
; CHECK-NEXT:   br i1 %cmp, label %return, label %if.end
; CHECK-NEXT: if.end:
; CHECK-NEXT:   %sub = add i32 %n, -1
; CHECK-NEXT:   %call = call i32 @fact(i32 %sub)
; CHECK-NEXT:   %mul = mul i32 %call, %n
; CHECK-NEXT:   ret i32 %mul
; CHECK-NEXT: return:
; CHECK-NEXT:   ret i32 %n
; CHECK-NEXT: }

define internal i32 @redirect(i32 %n) {
entry:
  %call = tail call i32 @redirect_target(i32 %n)
  ret i32 %call
}

; CHECK-NEXT: define internal i32 @redirect(i32 %n) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %call = call i32 @redirect_target(i32 %n)
; CHECK-NEXT:   ret i32 %call
; CHECK-NEXT: }

declare i32 @redirect_target(i32)

define internal void @call_void(i32 %n) {
entry:
  %cmp2 = icmp sgt i32 %n, 0
  br i1 %cmp2, label %if.then, label %if.end

if.then:                                          ; preds = %entry, %if.then
  %n.tr3 = phi i32 [ %call.i, %if.then ], [ %n, %entry ]
  %sub = add i32 %n.tr3, -1
  %call.i = tail call i32 @redirect_target(i32 %sub)
  %cmp = icmp sgt i32 %call.i, 0
  br i1 %cmp, label %if.then, label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; CHECK-NEXT: define internal void @call_void(i32 %n) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %cmp2 = icmp sgt i32 %n, 0
; CHECK-NEXT:   br i1 %cmp2, label %if.then, label %if.end
; CHECK-NEXT: if.then:
; CHECK-NEXT:   %n.tr3 = phi i32 [ %call.i, %if.then ], [ %n, %entry ]
; CHECK-NEXT:   %sub = add i32 %n.tr3, -1
; CHECK-NEXT:   %call.i = call i32 @redirect_target(i32 %sub)
; CHECK-NEXT:   %cmp = icmp sgt i32 %call.i, 0
; CHECK-NEXT:   br i1 %cmp, label %if.then, label %if.end
; CHECK-NEXT: if.end:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; NOIR: Total across all functions
