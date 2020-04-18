; This is a basic test of the alloca instruction.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

; Test that a sequence of allocas with less than stack alignment get fused.
define internal void @fused_small_align(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 4
  %a2 = alloca i8, i32 12, align 4
  %a3 = alloca i8, i32 16, align 8
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  ret void
}
; CHECK-LABEL: fused_small_align
; CHECK-NEXT: sub    esp,0x3c
; CHECK-NEXT: mov    eax,DWORD PTR [esp+0x40]
; CHECK-NEXT: mov    DWORD PTR [esp+0x10],eax
; CHECK-NEXT: mov    DWORD PTR [esp+0x18],eax
; CHECK-NEXT: mov    DWORD PTR [esp],eax
; CHECK-NEXT: add    esp,0x3c
; MIPS32-LABEL: fused_small_align
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	a0,{{.*}}(sp)
; MIPS32: 	addiu	sp,sp,{{.*}}

; Test that a sequence of allocas with greater than stack alignment get fused.
define internal void @fused_large_align(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 32
  %a2 = alloca i8, i32 12, align 64
  %a3 = alloca i8, i32 16, align 32
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  ret void
}
; CHECK-LABEL: fused_large_align
; CHECK-NEXT: push   ebp
; CHECK-NEXT: mov    ebp,esp
; CHECK-NEXT: sub    esp,0xb8
; CHECK-NEXT: and    esp,0xffffffc0
; CHECK-NEXT: mov    eax,DWORD PTR [ebp+0x8]
; CHECK-NEXT: mov    DWORD PTR [esp+0x40],eax
; CHECK-NEXT: mov    DWORD PTR [esp],eax
; CHECK-NEXT: mov    DWORD PTR [esp+0x60],eax
; CHECK-NEXT: mov    esp,ebp
; CHECK-NEXT: pop    ebp
; MIPS32-LABEL: fused_large_align
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	sw	s8,{{.*}}(sp)
; MIPS32: 	move	s8,sp
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	a0,{{.*}}(sp)
; MIPS32: 	move	sp,s8
; MIPS32: 	lw	s8,{{.*}}(sp)
; MIPS32: 	addiu	sp,sp,{{.*}}

; Test that an interior pointer into a rematerializable variable is also
; rematerializable, and test that it is detected even when the use appears
; syntactically before the definition.  Test that it is folded into mem
; operands, and also rematerializable through an lea instruction for direct use.
define internal i32 @fused_derived(i32 %arg) {
entry:
  %a1 = alloca i8, i32 128, align 4
  %a2 = alloca i8, i32 128, align 4
  %a3 = alloca i8, i32 128, align 4
  br label %block2
block1:
  %a2_i32 = bitcast i8* %a2 to i32*
  store i32 %arg, i32* %a2_i32, align 1
  store i32 %arg, i32* %derived, align 1
  ret i32 %retval
block2:
; The following are all rematerializable variables deriving from %a2.
  %p2 = ptrtoint i8* %a2 to i32
  %d = add i32 %p2, 12
  %retval = add i32 %p2, 1
  %derived = inttoptr i32 %d to i32*
  br label %block1
}
; CHECK-LABEL: fused_derived
; CHECK-NEXT: sub    esp,0x18c
; CHECK-NEXT: mov    [[ARG:e..]],DWORD PTR [esp+0x190]
; CHECK-NEXT: jmp
; CHECK-NEXT: mov    DWORD PTR [esp+0x80],[[ARG]]
; CHECK-NEXT: mov    DWORD PTR [esp+0x8c],[[ARG]]
; CHECK-NEXT: lea    eax,[esp+0x81]
; CHECK-NEXT: add    esp,0x18c
; CHECK-NEXT: ret
; MIPS32-LABEL: fused_derived
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	b
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	sw	a0,{{.*}}(sp)
; MIPS32: 	addiu	v0,sp,129
; MIPS32: 	addiu	sp,sp,{{.*}}

; Test that a fixed alloca gets referenced by the frame pointer.
define internal void @fused_small_align_with_dynamic(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 16
  br label %next
next:
  %a2 = alloca i8, i32 12, align 1
  %a3 = alloca i8, i32 16, align 1
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  ret void
}
; CHECK-LABEL: fused_small_align_with_dynamic
; CHECK-NEXT: push   ebp
; CHECK-NEXT: mov    ebp,esp
; CHECK-NEXT: sub    esp,0x18
; CHECK-NEXT: mov    eax,DWORD PTR [ebp+0x8]
; CHECK-NEXT: sub    esp,0x10
; CHECK-NEXT: mov    ecx,esp
; CHECK-NEXT: sub    esp,0x10
; CHECK-NEXT: mov    edx,esp
; CHECK-NEXT: mov    DWORD PTR [ebp-0x18],eax
; CHECK-NEXT: mov    DWORD PTR [ecx],eax
; CHECK-NEXT: mov    DWORD PTR [edx],eax
; CHECK-NEXT: mov    esp,ebp
; CHECK-NEXT: pop    ebp
; MIPS32-LABEL: fused_small_align_with_dynamic
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	sw	s8,{{.*}}(sp)
; MIPS32: 	move	s8,sp
; MIPS32: 	addiu	v0,sp,0
; MIPS32: 	addiu	v1,sp,16
; MIPS32: 	move	a1,a0
; MIPS32: 	sw	a1,32(s8)
; MIPS32: 	move	a1,a0
; MIPS32: 	sw	a1,0(v0)
; MIPS32: 	sw	a0,0(v1)
; MIPS32: 	move	sp,s8
; MIPS32: 	lw	s8,{{.*}}(sp)
; MIPS32: 	addiu	sp,sp,{{.*}}

; Test that a sequence with greater than stack alignment and dynamic size
; get folded and referenced correctly;

define internal void @fused_large_align_with_dynamic(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 32
  %a2 = alloca i8, i32 12, align 32
  %a3 = alloca i8, i32 16, align 1
  %a4 = alloca i8, i32 16, align 1
  br label %next
next:
  %a5 = alloca i8, i32 16, align 1
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  %p4 = bitcast i8* %a4 to i32*
  %p5 = bitcast i8* %a5 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  store i32 %arg, i32* %p4, align 1
  store i32 %arg, i32* %p5, align 1
  ret void
}
; CHECK-LABEL: fused_large_align_with_dynamic
; CHECK-NEXT: push   ebx
; CHECK-NEXT: push   ebp
; CHECK-NEXT: mov    ebp,esp
; CHECK-NEXT: sub    esp,0x24
; CHECK-NEXT: mov    eax,DWORD PTR [ebp+0xc]
; CHECK-NEXT: and    esp,0xffffffe0
; CHECK-NEXT: sub    esp,0x40
; CHECK-NEXT: mov    ecx,esp
; CHECK-NEXT: mov    edx,ecx
; CHECK-NEXT: add    ecx,0x20
; CHECK-NEXT: add    edx,0x0
; CHECK-NEXT: sub    esp,0x10
; CHECK-NEXT: mov    ebx,esp
; CHECK-NEXT: mov    DWORD PTR [edx],eax
; CHECK-NEXT: mov    DWORD PTR [ecx],eax
; CHECK-NEXT: mov    DWORD PTR [ebp-0x14],eax
; CHECK-NEXT: mov    DWORD PTR [ebp-0x24],eax
; CHECK-NEXT: mov    DWORD PTR [ebx],eax
; CHECK-NEXT: mov    esp,ebp
; CHECK-NEXT: pop    ebp
; MIPS32-LABEL: fused_large_align_with_dynamic
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	sw	s8,{{.*}}(sp)
; MIPS32: 	move	s8,sp
; MIPS32: 	addiu	v0,sp,0
; MIPS32: 	addiu	v1,sp,64
; MIPS32: 	move	a1,v0
; MIPS32: 	move	a2,a0
; MIPS32: 	sw	a2,0(a1)
; MIPS32: 	move	a1,a0
; MIPS32: 	sw	a1,32(v0)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,80(s8)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,96(s8)
; MIPS32: 	sw	a0,0(v1)
; MIPS32: 	move	sp,s8
; MIPS32: 	lw	s8,{{.*}}(sp)
; MIPS32: 	addiu	sp,sp,{{.*}}
