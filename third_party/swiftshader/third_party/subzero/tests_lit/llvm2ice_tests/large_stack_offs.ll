; This tries to create variables with very large stack offsets.
; This requires a lot of variables/register pressure. To simplify this
; we assume poor register allocation from Om1, and a flag that forces
; the frame to add K amount of unused stack for testing.
; We only need to test ARM and other architectures which have limited space
; for specifying an offset within an instruction.

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj --disassemble --target arm32 \
; RUN:   -i %s --args -Om1 --test-stack-extra 4096 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 %s

declare i64 @dummy(i32 %t1, i32 %t2, i32 %t3, i64 %t4, i64 %t5)

; Test a function that requires lots of stack (due to test flag), and uses
; SP as the base register (originally).
define internal i64 @lotsOfStack(i32 %a, i32 %b, i32 %c, i32 %d) {
entry:
  %t1 = xor i32 %a, %b
  %t2 = or i32 %c, %d
  %cmp = icmp eq i32 %t1, %t2
  br i1 %cmp, label %br_1, label %br_2

br_1:
  %x1 = zext i32 %t1 to i64
  %y1 = ashr i64 %x1, 17
  ; Use some stack during the call, so that references to %t1 and %t2's
  ; stack slots require stack adjustment.
  %r1 = call i64 @dummy(i32 123, i32 321, i32 %t2, i64 %x1, i64 %y1)
  %z1 = sub i64 %r1, %y1
  br label %end

br_2:
  %x2 = zext i32 %t2 to i64
  %y2 = and i64 %x2, 123
  %r2 = call i64 @dummy(i32 123, i32 321, i32 %t2, i64 %x2, i64 %y2)
  %z2 = and i64 %r2, %y2
  br label %end

end:
  %x3 = phi i64 [ %x1, %br_1 ], [ %x2, %br_2 ]
  %z3 = phi i64 [ %z1, %br_1 ], [ %z2, %br_2 ]
  %r3 = and i64 %x3, %z3
  ret i64 %r3
}
; ARM32-LABEL: lotsOfStack
; ARM32-NOT: mov fp, sp
; ARM32: movw ip, #4{{.*}}
; ARM32-NEXT: sub sp, sp, ip
; ARM32: movw ip, #4248
; ARM32-NEXT: add ip, sp, ip
; ARM32-NOT: movw ip
; %t2 is the result of the "or", and %t2 will be passed via r1 to the call.
; Use that to check the stack offset of %t2. The first offset and the
; later offset right before the call should be 16 bytes apart,
; because of the sub sp, sp, #16.
; ARM32: orr [[REG:r.*]], {{.*}},
; I.e., the slot for t2 is (sp0 + 4232 - 20) == sp0 + 4212.
; ARM32: str [[REG]], [ip, #-20]
; ARM32: b {{[a-f0-9]+}}
; Now skip ahead to where the call in br_1 begins, to check how %t2 is used.
; ARM32: movw ip, #4232
; ARM32-NEXT: add ip, sp, ip
; ARM32: ldr r2, [ip, #-4]
; ARM32: bl {{.*}} dummy
; The call clobbers ip, so we need to re-create the base register.
; ARM32: movw ip, #4{{.*}}
; ARM32: b {{[a-f0-9]+}}
; ARM32: bl {{.*}} dummy

; Similar, but test a function that uses FP as the base register (originally).
define internal i64 @usesFrameReg(i32 %a, i32 %b, i32 %c, i32 %d) {
entry:
  %p = alloca i8, i32 %d, align 4
  %t1 = xor i32 %a, %b
  %t2 = or i32 %c, %d
  %cmp = icmp eq i32 %t1, %t2
  br i1 %cmp, label %br_1, label %br_2

br_1:
  %x1 = zext i32 %t1 to i64
  %y1 = ashr i64 %x1, 17
  %p32 = ptrtoint i8* %p to i32
  %r1 = call i64 @dummy(i32 %p32, i32 321, i32 %t2, i64 %x1, i64 %y1)
  %z1 = sub i64 %r1, %y1
  br label %end

br_2:
  %x2 = zext i32 %t2 to i64
  %y2 = and i64 %x2, 123
  %r2 = call i64 @dummy(i32 123, i32 321, i32 %d, i64 %x2, i64 %y2)
  %z2 = and i64 %r2, %y2
  br label %end

end:
  %x3 = phi i64 [ %x1, %br_1 ], [ %x2, %br_2 ]
  %z3 = phi i64 [ %z1, %br_1 ], [ %z2, %br_2 ]
  %r3 = and i64 %x3, %z3
  ret i64 %r3
}
; ARM32-LABEL: usesFrameReg
; ARM32: mov fp, sp
; ARM32: movw ip, #4{{.*}}
; ARM32-NEXT: sub sp, sp, ip
; ARM32: movw ip, #4100
; ARM32-NEXT: sub ip, fp, ip
; ARM32-NOT: movw ip
; %t2 is the result of the "or", and %t2 will be passed via r1 to the call.
; Use that to check the stack offset of %t2. It should be the same offset
; even after sub sp, sp, #16, because the base register was originally
; the FP and not the SP.
; ARM32: orr [[REG:r.*]], {{.*}},
; I.e., the slot for t2 is (fp0 - 4100 -24) == fp0 - 4124
; ARM32: str [[REG]], [ip, #-24]
; ARM32: b {{[a-f0-9]+}}
; Now skip ahead to where the call in br_1 begins, to check how %t2 is used.
; ARM32: movw ip, #4120
; ARM32-NEXT: sub ip, fp, ip
; ARM32: ldr r2, [ip, #-4]
; ARM32: bl {{.*}} dummy
; The call clobbers ip, so we need to re-create the base register.
; ARM32: movw ip, #4{{.*}}
; ARM32: b {{[a-f0-9]+}}
; ARM32: bl {{.*}} dummy
