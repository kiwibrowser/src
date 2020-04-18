; This is a basic test of the alloca instruction and a call.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble --disassemble --target \
; RUN:   mips32 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 %s

declare void @copy(i32 %arg1, i8* %arr1, i8* %arr2, i8* %arr3, i8* %arr4);

; Test that alloca base addresses get passed correctly to functions.
define internal void @caller1(i32 %arg) {
entry:
  %a1 = alloca i8, i32 32, align 4
  %p1 = bitcast i8* %a1 to i32*
  store i32 %arg, i32* %p1, align 1
  call void @copy(i32 %arg, i8* %a1, i8* %a1, i8* %a1, i8* %a1)
  ret void
}

; CHECK-LABEL:  caller1
; CHECK-NEXT:   sub    esp,0x4c
; CHECK-NEXT:   mov    eax,DWORD PTR [esp+0x50]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x20],eax
; CHECK-NEXT:   mov    DWORD PTR [esp],eax
; CHECK-NEXT:   lea    eax,[esp+0x20]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x4],eax
; CHECK-NEXT:   lea    eax,[esp+0x20]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x8],eax
; CHECK-NEXT:   lea    eax,[esp+0x20]
; CHECK-NEXT:   mov    DWORD PTR [esp+0xc],eax
; CHECK-NEXT:   lea    eax,[esp+0x20]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x10],eax
; CHECK-NEXT:   call
; CHECK-NEXT:   add    esp,0x4c
; CHECK-NEXT:   ret
; MIPS32-LABEL: caller1
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	sw	ra,{{.*}}(sp)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	addiu	v0,sp,32
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	addiu	a1,sp,32
; MIPS32: 	addiu	a2,sp,32
; MIPS32: 	addiu	a3,sp,32
; MIPS32: 	jal
; MIPS32: 	nop
; MIPS32: 	lw	ra,{{.*}}(sp)
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	jr	ra

; Test that alloca base addresses get passed correctly to functions.
define internal void @caller2(i32 %arg) {
entry:
  %a1 = alloca i8, i32 32, align 4
  %a2 = alloca i8, i32 32, align 4
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  call void @copy(i32 %arg, i8* %a1, i8* %a2, i8* %a1, i8* %a2)
  ret void
}

; CHECK-LABEL:  caller2
; CHECK-NEXT:   sub    esp,0x6c
; CHECK-NEXT:   mov    eax,DWORD PTR [esp+0x70]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x20],eax
; CHECK-NEXT:   mov    DWORD PTR [esp+0x40],eax
; CHECK-NEXT:   mov    DWORD PTR [esp],eax
; CHECK-NEXT:   lea    eax,[esp+0x20]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x4],eax
; CHECK-NEXT:   lea    eax,[esp+0x40]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x8],eax
; CHECK-NEXT:   lea    eax,[esp+0x20]
; CHECK-NEXT:   mov    DWORD PTR [esp+0xc],eax
; CHECK-NEXT:   lea    eax,[esp+0x40]
; CHECK-NEXT:   mov    DWORD PTR [esp+0x10],eax
; CHECK-NEXT:   call
; CHECK-NEXT:   add    esp,0x6c
; CHECK-NEXT:   ret
; MIPS32-LABEL: caller2
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	sw	ra,{{.*}}(sp)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	move	v0,a0
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	addiu	v0,sp,64
; MIPS32: 	sw	v0,{{.*}}(sp)
; MIPS32: 	addiu	a1,sp,32
; MIPS32: 	addiu	a2,sp,64
; MIPS32: 	addiu	a3,sp,32
; MIPS32: 	jal
; MIPS32: 	nop
; MIPS32: 	lw	ra,{{.*}}(sp)
; MIPS32: 	addiu	sp,sp,{{.*}}
; MIPS32: 	jr	ra
