; This is a basic test of the alloca instruction.

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -O2 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck %s

; RUN: %if --need=target_X8632 --command %p2i --filetype=obj --disassemble \
; RUN:   --target x8632 -i %s --args -Om1 -allow-externally-defined-symbols \
; RUN:   | %if --need=target_X8632 --command FileCheck \
; RUN:   --check-prefix CHECK-OPTM1 %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix=ARM-OPT2 %s

; RUN: %if --need=target_ARM32 \
; RUN:   --command %p2i --filetype=obj \
; RUN:   --disassemble --target arm32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_ARM32 \
; RUN:   --command FileCheck --check-prefix ARM32 --check-prefix=ARM-OPTM1 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 --check-prefix=MIPS32-OPT2 %s

; RUN: %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command %p2i --filetype=asm --assemble \
; RUN:   --disassemble --target mips32 -i %s --args -Om1 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=target_MIPS32 --need=allow_dump \
; RUN:   --command FileCheck --check-prefix MIPS32 --check-prefix=MIPS32-OPTM1 %s

define internal void @fixed_416_align_16(i32 %n) {
entry:
  %array = alloca i8, i32 416, align 16
  %__2 = ptrtoint i8* %array to i32
  call void @f1(i32 %__2)
  ret void
}
; CHECK-LABEL: fixed_416_align_16
; CHECK:      sub     esp,0x1bc
; CHECK:      lea     eax,[esp+0x10]
; CHECK:      mov     DWORD PTR [esp],eax
; CHECK:      call {{.*}} R_{{.*}}    f1

; CHECK-OPTM1-LABEL: fixed_416_align_16
; CHECK-OPTM1:      sub     esp,0x18
; CHECK-OPTM1:      sub     esp,0x1a0
; CHECK-OPTM1:      mov     DWORD PTR [esp],eax
; CHECK-OPTM1:      call {{.*}} R_{{.*}}    f1

; ARM32-LABEL: fixed_416_align_16
; ARM32-OPT2:  sub sp, sp, #428
; ARM32-OPTM1: sub sp, sp, #416
; ARM32:       bl {{.*}} R_{{.*}}    f1

; MIPS32-LABEL: fixed_416_align_16
; MIPS32-OPT2: addiu sp,sp,-448
; MIPS32-OPT2: addiu a0,sp,16
; MIPS32-OPTM1: addiu sp,sp,-464
; MIPS32-OPTM1: addiu [[REG:.*]],sp,16
; MIPS32-OPTM1: sw [[REG]],{{.*}}
; MIPS32-OPTM1: lw a0,{{.*}}
; MIPS32: jal {{.*}} R_{{.*}} f1

define internal void @fixed_416_align_32(i32 %n) {
entry:
  %array = alloca i8, i32 400, align 32
  %__2 = ptrtoint i8* %array to i32
  call void @f1(i32 %__2)
  ret void
}
; CHECK-LABEL: fixed_416_align_32
; CHECK:      push    ebp
; CHECK-NEXT: mov     ebp,esp
; CHECK:      sub     esp,0x1d8
; CHECK:      and     esp,0xffffffe0
; CHECK:      lea     eax,[esp+0x10]
; CHECK:      mov     DWORD PTR [esp],eax
; CHECK:      call {{.*}} R_{{.*}}    f1

; ARM32-LABEL: fixed_416_align_32
; ARM32-OPT2:  sub sp, sp, #424
; ARM32-OPTM1: sub sp, sp, #416
; ARM32:       bic sp, sp, #31
; ARM32:       bl {{.*}} R_{{.*}}    f1

; MIPS32-LABEL: fixed_416_align_32
; MIPS32-OPT2: addiu sp,sp,-448
; MIPS32-OPT2: addiu a0,sp,16
; MIPS32-OPTM1: addiu sp,sp,-464
; MIPS32-OPTM1: addiu [[REG:.*]],sp,32
; MIPS32-OPTM1: sw [[REG]],{{.*}}
; MIPS32-OPTM1: lw a0,{{.*}}
; MIPS32: jal {{.*}} R_{{.*}} f1

; Show that the amount to allocate will be rounded up.
define internal void @fixed_351_align_16(i32 %n) {
entry:
  %array = alloca i8, i32 351, align 16
  %__2 = ptrtoint i8* %array to i32
  call void @f1(i32 %__2)
  ret void
}
; CHECK-LABEL: fixed_351_align_16
; CHECK:      sub     esp,0x17c
; CHECK:      lea     eax,[esp+0x10]
; CHECK:      mov     DWORD PTR [esp],eax
; CHECK:      call {{.*}} R_{{.*}}    f1

; CHECK-OPTM1-LABEL: fixed_351_align_16
; CHECK-OPTM1:      sub     esp,0x18
; CHECK-OPTM1:      sub     esp,0x160
; CHECK-OPTM1:      mov     DWORD PTR [esp],eax
; CHECK-OPTM1:      call {{.*}} R_{{.*}}    f1

; ARM32-LABEL: fixed_351_align_16
; ARM32-OPT2:  sub sp, sp, #364
; ARM32-OPTM1: sub sp, sp, #352
; ARM32:       bl {{.*}} R_{{.*}}    f1

; MIPS32-LABEL: fixed_351_align_16
; MIPS32-OPT2: addiu sp,sp,-384
; MIPS32-OPT2: addiu a0,sp,16
; MIPS32-OPTM1: addiu sp,sp,-400
; MIPS32-OPTM1: addiu [[REG:.*]],sp,16
; MIPS32-OPTM1: sw [[REG]],{{.*}}
; MIPS32-OPTM1: lw a0,{{.*}}
; MIPS32: jal {{.*}} R_{{.*}} f1

define internal void @fixed_351_align_32(i32 %n) {
entry:
  %array = alloca i8, i32 351, align 32
  %__2 = ptrtoint i8* %array to i32
  call void @f1(i32 %__2)
  ret void
}
; CHECK-LABEL: fixed_351_align_32
; CHECK:      push    ebp
; CHECK-NEXT: mov     ebp,esp
; CHECK:      sub     esp,0x198
; CHECK:      and     esp,0xffffffe0
; CHECK:      lea     eax,[esp+0x10]
; CHECK:      mov     DWORD PTR [esp],eax
; CHECK:      call {{.*}} R_{{.*}}    f1

; ARM32-LABEL: fixed_351_align_32
; ARM32-OPT2:  sub sp, sp, #360
; ARM32-OPTM1: sub sp, sp, #352
; ARM32:       bic sp, sp, #31
; ARM32:       bl {{.*}} R_{{.*}}    f1

; MIPS32-LABEL: fixed_351_align_32
; MIPS32-OPT2: addiu sp,sp,-384
; MIPS32-OPT2: addiu a0,sp,16
; MIPS32-OPTM1: addiu sp,sp,-400
; MIPS32-OPTM1: addiu [[REG:.*]],sp,32
; MIPS32-OPTM1: sw [[REG]],{{.*}}
; MIPS32-OPTM1: lw a0,{{.*}}
; MIPS32: jal {{.*}} R_{{.*}} f1

declare void @f1(i32 %ignored)

declare void @f2(i32 %ignored)

define internal void @variable_n_align_16(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 16
  %__2 = ptrtoint i8* %array to i32
  call void @f2(i32 %__2)
  ret void
}
; CHECK-LABEL: variable_n_align_16
; CHECK:      sub     esp,0x18
; CHECK:      mov     eax,DWORD PTR [ebp+0x8]
; CHECK:      add     eax,0xf
; CHECK:      and     eax,0xfffffff0
; CHECK:      sub     esp,eax
; CHECK:      lea     eax,[esp+0x10]
; CHECK:      mov     DWORD PTR [esp],eax
; CHECK:      call {{.*}} R_{{.*}}    f2

; ARM32-LABEL: variable_n_align_16
; ARM32:      add r0, r0, #15
; ARM32:      bic r0, r0, #15
; ARM32:      sub sp, sp, r0
; ARM32:      bl {{.*}} R_{{.*}}    f2

; MIPS32-LABEL: variable_n_align_16
; MIPS32: addiu	[[REG:.*]],{{.*}},15
; MIPS32: li	[[REG1:.*]],-16
; MIPS32: and	[[REG2:.*]],[[REG]],[[REG1]]
; MIPS32: subu	[[REG3:.*]],sp,[[REG2:.*]]
; MIPS32: li	[[REG4:.*]],-16
; MIPS32: and	{{.*}},[[REG3]],[[REG4]]
; MIPS32: addiu	sp,sp,-16
; MIPS32: jal	{{.*}} R_{{.*}} f2
; MIPS32: addiu	sp,sp,16

define internal void @variable_n_align_32(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 32
  %__2 = ptrtoint i8* %array to i32
  call void @f2(i32 %__2)
  ret void
}
; In -O2, the order of the CHECK-DAG lines in the output is switched.
; CHECK-LABEL: variable_n_align_32
; CHECK:      push    ebp
; CHECK:      mov     ebp,esp
; CHECK:      sub     esp,0x18
; CHECK-DAG:  and     esp,0xffffffe0
; CHECK-DAG:  mov     eax,DWORD PTR [ebp+0x8]
; CHECK:      add     eax,0x1f
; CHECK:      and     eax,0xffffffe0
; CHECK:      sub     esp,eax
; CHECK:      lea     eax,[esp+0x10]
; CHECK:      mov     DWORD PTR [esp],eax
; CHECK:      call {{.*}} R_{{.*}}    f2
; CHECK:      mov     esp,ebp
; CHECK:      pop     ebp

; ARM32-LABEL: variable_n_align_32
; ARM32:      push {fp, lr}
; ARM32:      mov fp, sp
; ARM32:      bic sp, sp, #31
; ARM32:      add r0, r0, #31
; ARM32:      bic r0, r0, #31
; ARM32:      sub sp, sp, r0
; ARM32:      bl {{.*}} R_{{.*}}    f2
; ARM32:      mov sp, fp
; ARM32:      pop {fp, lr}

; MIPS32-LABEL: variable_n_align_32
; MIPS32: addiu	[[REG:.*]],{{.*}},15
; MIPS32: li 	[[REG1:.*]],-16
; MIPS32: and 	[[REG2:.*]],[[REG]],[[REG1]]
; MIPS32: subu 	[[REG3:.*]],sp,[[REG2]]
; MIPS32: li 	[[REG4:.*]],-32
; MIPS32: and 	{{.*}},[[REG3]],[[REG4]]
; MIPS32: addiu	sp,sp,-16
; MIPS32: jal 	{{.*}} R_{{.*}} f2
; MIPS32: addiu	sp,sp,16

; Test alloca with default (0) alignment.
define internal void @align0(i32 %n) {
entry:
  %array = alloca i8, i32 %n
  %__2 = ptrtoint i8* %array to i32
  call void @f2(i32 %__2)
  ret void
}
; CHECK-LABEL: align0
; CHECK: add [[REG:.*]],0xf
; CHECK: and [[REG]],0xfffffff0
; CHECK: sub esp,[[REG]]

; ARM32-LABEL: align0
; ARM32: add r0, r0, #15
; ARM32: bic r0, r0, #15
; ARM32: sub sp, sp, r0

; MIPS32-LABEL: align0
; MIPS32: addiu	[[REG:.*]],{{.*}},15
; MIPS32: li	[[REG1:.*]],-16
; MIPS32: and	[[REG2:.*]],[[REG]],[[REG1]]
; MIPS32: subu	{{.*}},sp,[[REG2]]
; MIPS32: addiu	sp,sp,-16
; MIPS32: jal	{{.*}} R_{{.*}} f2
; MIPS32: addiu	sp,sp,16

; Test a large alignment where a mask might not fit in an immediate
; field of an instruction for some architectures.
define internal void @align1MB(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 1048576
  %__2 = ptrtoint i8* %array to i32
  call void @f2(i32 %__2)
  ret void
}
; CHECK-LABEL: align1MB
; CHECK: push ebp
; CHECK-NEXT: mov ebp,esp
; CHECK: and esp,0xfff00000
; CHECK: add [[REG:.*]],0xfffff
; CHECK: and [[REG]],0xfff00000
; CHECK: sub esp,[[REG]]

; ARM32-LABEL: align1MB
; ARM32: movw [[REG:.*]], #0
; ARM32: movt [[REG]], #65520 ; 0xfff0
; ARM32: and sp, sp, [[REG]]
; ARM32: movw [[REG2:.*]], #65535 ; 0xffff
; ARM32: movt [[REG2]], #15
; ARM32: add r0, r0, [[REG2]]
; ARM32: movw [[REG3:.*]], #0
; ARM32: movt [[REG3]], #65520 ; 0xfff0
; ARM32: and r0, r0, [[REG3]]
; ARM32: sub sp, sp, r0

; MIPS32-LABEL: align1MB
; MIPS32: addiu	[[REG:.*]],{{.*}},15
; MIPS32: li	[[REG1:.*]],-16
; MIPS32: and	[[REG2:.*]],[[REG]],[[REG1]]
; MIPS32: subu	[[REG3:.*]],sp,[[REG2]]
; MIPS32: lui	[[REG4:.*]],0xfff0
; MIPS32: and	{{.*}},[[REG3]],[[REG4]]
; MIPS32: addiu	sp,sp,-16
; MIPS32: jal   {{.*}} R_{{.*}} f2
; MIPS32: addiu	sp,sp,16

; Test a large alignment where a mask might still fit in an immediate
; field of an instruction for some architectures.
define internal void @align512MB(i32 %n) {
entry:
  %array = alloca i8, i32 %n, align 536870912
  %__2 = ptrtoint i8* %array to i32
  call void @f2(i32 %__2)
  ret void
}
; CHECK-LABEL: align512MB
; CHECK: push ebp
; CHECK-NEXT: mov ebp,esp
; CHECK: and esp,0xe0000000
; CHECK: add [[REG:.*]],0x1fffffff
; CHECK: and [[REG]],0xe0000000
; CHECK: sub esp,[[REG]]

; ARM32-LABEL: align512MB
; ARM32: and sp, sp, #-536870912 ; 0xe0000000
; ARM32: mvn [[REG:.*]], #-536870912 ; 0xe0000000
; ARM32: add r0, r0, [[REG]]
; ARM32: and r0, r0, #-536870912 ; 0xe0000000
; ARM32: sub sp, sp, r0

; MIPS32-LABEL: align512MB
; MIPS32: addiu	[[REG:.*]],{{.*}},15
; MIPS32: li	[[REG2:.*]],-16
; MIPS32: and	[[REG3:.*]],[[REG]],[[REG2]]
; MIPS32: subu	[[REG4:.*]],sp,[[REG3]]
; MIPS32: lui	[[REG5:.*]],0xe000
; MIPS32: and	{{.*}},[[REG4]],[[REG5]]
; MIPS32: addiu	sp,sp,-16
; MIPS32: jal	{{.*}} R_{{.*}} f2
; MIPS32: addiu	sp,sp,16

; Test that a simple alloca sequence doesn't trigger a frame pointer.
define internal void @fixed_no_frameptr(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 4
  %a2 = alloca i8, i32 12, align 4
  %a3 = alloca i8, i32 16, align 4
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  ret void
}
; CHECK-LABEL: fixed_no_frameptr
; CHECK-NOT:      mov     ebp,esp

; Test that a simple alloca sequence with at least one large alignment does
; trigger a frame pointer.
define internal void @fixed_bigalign_with_frameptr(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 4
  %a2 = alloca i8, i32 12, align 4
  %a3 = alloca i8, i32 16, align 64
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  ret void
}
; CHECK-LABEL: fixed_bigalign_with_frameptr
; CHECK:      mov     ebp,esp

; Test that a more complex alloca sequence does trigger a frame pointer.
define internal void @var_with_frameptr(i32 %arg) {
entry:
  %a1 = alloca i8, i32 8, align 4
  %a2 = alloca i8, i32 12, align 4
  %a3 = alloca i8, i32 %arg, align 4
  %p1 = bitcast i8* %a1 to i32*
  %p2 = bitcast i8* %a2 to i32*
  %p3 = bitcast i8* %a3 to i32*
  store i32 %arg, i32* %p1, align 1
  store i32 %arg, i32* %p2, align 1
  store i32 %arg, i32* %p3, align 1
  ret void
}
; CHECK-LABEL: var_with_frameptr
; CHECK:      mov     ebp,esp
