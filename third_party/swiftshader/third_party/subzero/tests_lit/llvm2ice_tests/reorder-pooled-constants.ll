; This is a smoke test for reordering pooled constants.
; This option is only implemented for target X8632 for now.

; RUN: %p2i --assemble --disassemble --filetype=obj --dis-flags=-s \
; RUN:   --target x8632 -i %s --args -sz-seed=1 -O2 -reorder-pooled-constants \
; RUN:   -allow-externally-defined-symbols | FileCheck %s --check-prefix=X86

; RUN: %p2i --assemble --disassemble --filetype=obj --dis-flags=-s \
; RUN:   --target x8632 -i %s --args -sz-seed=1 -Om1 -reorder-pooled-constants \
; RUN:   -allow-externally-defined-symbols | FileCheck %s --check-prefix=X86

@__init_array_start = internal constant [0 x i8] zeroinitializer, align 4
@__fini_array_start = internal constant [0 x i8] zeroinitializer, align 4
@__tls_template_start = internal constant [0 x i8] zeroinitializer, align 8
@__tls_template_alignment = internal constant [4 x i8] c"\01\00\00\00", align 4

define internal float @FpLookup1(i32 %Arg) {
entry:
  switch i32 %Arg, label %return [
    i32 0, label %sw.bb
    i32 1, label %sw.bb1
    i32 2, label %sw.bb4
    i32 3, label %sw.bb7
    i32 -1, label %sw.bb10
    i32 -2, label %sw.bb14
    i32 -3, label %sw.bb19
    i32 10, label %sw.bb24
    i32 -10, label %sw.bb27
    i32 100, label %sw.bb30
    i32 101, label %sw.bb33
    i32 102, label %sw.bb36
    i32 103, label %sw.bb39
    i32 -101, label %sw.bb42
    i32 -102, label %sw.bb47
    i32 -103, label %sw.bb52
    i32 110, label %sw.bb57
    i32 -110, label %sw.bb60
  ]

sw.bb:                                            ; preds = %entry
  %call = call float @Dummy(i32 0)
  %add = fadd float %call, 1.000000e+00
  br label %return

sw.bb1:                                           ; preds = %entry
  %call2 = call float @Dummy(i32 1)
  %add3 = fadd float %call2, 2.000000e+00
  br label %return

sw.bb4:                                           ; preds = %entry
  %call5 = call float @Dummy(i32 2)
  %add6 = fadd float %call5, 4.000000e+00
  br label %return

sw.bb7:                                           ; preds = %entry
  %call8 = call float @Dummy(i32 3)
  %add9 = fadd float %call8, 8.000000e+00
  br label %return

sw.bb10:                                          ; preds = %entry
  %call11 = call float @Dummy(i32 -1)
  %conv13 = fadd float %call11, 5.000000e-01
  br label %return

sw.bb14:                                          ; preds = %entry
  %call15 = call float @Dummy(i32 -2)
  %conv16 = fpext float %call15 to double
  %add17 = fadd double %conv16, 0x3FD5555555555555
  %conv18 = fptrunc double %add17 to float
  br label %return

sw.bb19:                                          ; preds = %entry
  %call20 = call float @Dummy(i32 -3)
  %conv23 = fadd float %call20, 2.500000e-01
  br label %return

sw.bb24:                                          ; preds = %entry
  %call25 = call float @Dummy(i32 10)
  %add26 = fadd float %call25, 0x7FF8000000000000
  br label %return

sw.bb27:                                          ; preds = %entry
  %call28 = call float @Dummy(i32 -10)
  %add29 = fadd float %call28, 0xFFF8000000000000
  br label %return

sw.bb30:                                          ; preds = %entry
  %call31 = call float @Dummy(i32 100)
  %add32 = fadd float %call31, 1.000000e+00
  br label %return

sw.bb33:                                          ; preds = %entry
  %call34 = call float @Dummy(i32 101)
  %add35 = fadd float %call34, 2.000000e+00
  br label %return

sw.bb36:                                          ; preds = %entry
  %call37 = call float @Dummy(i32 102)
  %add38 = fadd float %call37, 4.000000e+00
  br label %return

sw.bb39:                                          ; preds = %entry
  %call40 = call float @Dummy(i32 103)
  %add41 = fadd float %call40, 8.000000e+00
  br label %return

sw.bb42:                                          ; preds = %entry
  %call43 = call float @Dummy(i32 -101)
  %conv46 = fadd float %call43, 5.000000e-01
  br label %return

sw.bb47:                                          ; preds = %entry
  %call48 = call float @Dummy(i32 -102)
  %conv49 = fpext float %call48 to double
  %add50 = fadd double %conv49, 0x3FD5555555555555
  %conv51 = fptrunc double %add50 to float
  br label %return

sw.bb52:                                          ; preds = %entry
  %call53 = call float @Dummy(i32 -103)
  %conv56 = fadd float %call53, 2.500000e-01
  br label %return

sw.bb57:                                          ; preds = %entry
  %call58 = call float @Dummy(i32 110)
  %add59 = fadd float %call58, 0x7FF8000000000000
  br label %return

sw.bb60:                                          ; preds = %entry
  %call61 = call float @Dummy(i32 -110)
  %add62 = fadd float %call61, 0xFFF8000000000000
  br label %return

return:                                           ; preds = %entry, %sw.bb60, %sw.bb57, %sw.bb52, %sw.bb47, %sw.bb42, %sw.bb39, %sw.bb36, %sw.bb33, %sw.bb30, %sw.bb27, %sw.bb24, %sw.bb19, %sw.bb14, %sw.bb10, %sw.bb7, %sw.bb4, %sw.bb1, %sw.bb
  %retval.0 = phi float [ %add62, %sw.bb60 ], [ %add59, %sw.bb57 ], [ %conv56, %sw.bb52 ], [ %conv51, %sw.bb47 ], [ %conv46, %sw.bb42 ], [ %add41, %sw.bb39 ], [ %add38, %sw.bb36 ], [ %add35, %sw.bb33 ], [ %add32, %sw.bb30 ], [ %add29, %sw.bb27 ], [ %add26, %sw.bb24 ], [ %conv23, %sw.bb19 ], [ %conv18, %sw.bb14 ], [ %conv13, %sw.bb10 ], [ %add9, %sw.bb7 ], [ %add6, %sw.bb4 ], [ %add3, %sw.bb1 ], [ %add, %sw.bb ], [ 0.000000e+00, %entry ]
  ret float %retval.0
}

declare float @Dummy(i32)

define internal float @FpLookup2(i32 %Arg) {
entry:
  switch i32 %Arg, label %return [
    i32 0, label %sw.bb
    i32 1, label %sw.bb1
    i32 2, label %sw.bb4
    i32 3, label %sw.bb7
    i32 -1, label %sw.bb10
    i32 -2, label %sw.bb14
    i32 -3, label %sw.bb19
    i32 10, label %sw.bb24
    i32 -10, label %sw.bb27
    i32 100, label %sw.bb30
    i32 101, label %sw.bb33
    i32 102, label %sw.bb36
    i32 103, label %sw.bb39
    i32 -101, label %sw.bb42
    i32 -102, label %sw.bb47
    i32 -103, label %sw.bb52
    i32 110, label %sw.bb57
    i32 -110, label %sw.bb60
  ]

sw.bb:                                            ; preds = %entry
  %call = call float @Dummy(i32 0)
  %add = fadd float %call, 1.000000e+00
  br label %return

sw.bb1:                                           ; preds = %entry
  %call2 = call float @Dummy(i32 1)
  %add3 = fadd float %call2, 2.000000e+00
  br label %return

sw.bb4:                                           ; preds = %entry
  %call5 = call float @Dummy(i32 2)
  %add6 = fadd float %call5, 4.000000e+00
  br label %return

sw.bb7:                                           ; preds = %entry
  %call8 = call float @Dummy(i32 3)
  %add9 = fadd float %call8, 8.000000e+00
  br label %return

sw.bb10:                                          ; preds = %entry
  %call11 = call float @Dummy(i32 -1)
  %conv13 = fadd float %call11, 5.000000e-01
  br label %return

sw.bb14:                                          ; preds = %entry
  %call15 = call float @Dummy(i32 -2)
  %conv16 = fpext float %call15 to double
  %add17 = fadd double %conv16, 0x3FD5555555555555
  %conv18 = fptrunc double %add17 to float
  br label %return

sw.bb19:                                          ; preds = %entry
  %call20 = call float @Dummy(i32 -3)
  %conv23 = fadd float %call20, 2.500000e-01
  br label %return

sw.bb24:                                          ; preds = %entry
  %call25 = call float @Dummy(i32 10)
  %add26 = fadd float %call25, 0x7FF8000000000000
  br label %return

sw.bb27:                                          ; preds = %entry
  %call28 = call float @Dummy(i32 -10)
  %add29 = fadd float %call28, 0xFFF8000000000000
  br label %return

sw.bb30:                                          ; preds = %entry
  %call31 = call float @Dummy(i32 100)
  %add32 = fadd float %call31, 1.000000e+00
  br label %return

sw.bb33:                                          ; preds = %entry
  %call34 = call float @Dummy(i32 101)
  %add35 = fadd float %call34, 2.000000e+00
  br label %return

sw.bb36:                                          ; preds = %entry
  %call37 = call float @Dummy(i32 102)
  %add38 = fadd float %call37, 4.000000e+00
  br label %return

sw.bb39:                                          ; preds = %entry
  %call40 = call float @Dummy(i32 103)
  %add41 = fadd float %call40, 8.000000e+00
  br label %return

sw.bb42:                                          ; preds = %entry
  %call43 = call float @Dummy(i32 -101)
  %conv46 = fadd float %call43, 5.000000e-01
  br label %return

sw.bb47:                                          ; preds = %entry
  %call48 = call float @Dummy(i32 -102)
  %conv49 = fpext float %call48 to double
  %add50 = fadd double %conv49, 0x3FD5555555555555
  %conv51 = fptrunc double %add50 to float
  br label %return

sw.bb52:                                          ; preds = %entry
  %call53 = call float @Dummy(i32 -103)
  %conv56 = fadd float %call53, 2.500000e-01
  br label %return

sw.bb57:                                          ; preds = %entry
  %call58 = call float @Dummy(i32 110)
  %add59 = fadd float %call58, 0x7FF8000000000000
  br label %return

sw.bb60:                                          ; preds = %entry
  %call61 = call float @Dummy(i32 -110)
  %add62 = fadd float %call61, 0xFFF8000000000000
  br label %return

return:                                           ; preds = %entry, %sw.bb60, %sw.bb57, %sw.bb52, %sw.bb47, %sw.bb42, %sw.bb39, %sw.bb36, %sw.bb33, %sw.bb30, %sw.bb27, %sw.bb24, %sw.bb19, %sw.bb14, %sw.bb10, %sw.bb7, %sw.bb4, %sw.bb1, %sw.bb
  %retval.0 = phi float [ %add62, %sw.bb60 ], [ %add59, %sw.bb57 ], [ %conv56, %sw.bb52 ], [ %conv51, %sw.bb47 ], [ %conv46, %sw.bb42 ], [ %add41, %sw.bb39 ], [ %add38, %sw.bb36 ], [ %add35, %sw.bb33 ], [ %add32, %sw.bb30 ], [ %add29, %sw.bb27 ], [ %add26, %sw.bb24 ], [ %conv23, %sw.bb19 ], [ %conv18, %sw.bb14 ], [ %conv13, %sw.bb10 ], [ %add9, %sw.bb7 ], [ %add6, %sw.bb4 ], [ %add3, %sw.bb1 ], [ %add, %sw.bb ], [ 0.000000e+00, %entry ]
  ret float %retval.0
}

define internal double @FpLookup3(i32 %Arg) {
entry:
  switch i32 %Arg, label %return [
    i32 0, label %sw.bb
    i32 1, label %sw.bb1
    i32 2, label %sw.bb5
    i32 3, label %sw.bb9
    i32 -1, label %sw.bb13
    i32 -2, label %sw.bb17
    i32 -3, label %sw.bb21
    i32 10, label %sw.bb25
    i32 -10, label %sw.bb29
    i32 100, label %sw.bb33
    i32 101, label %sw.bb37
    i32 102, label %sw.bb41
    i32 103, label %sw.bb45
    i32 -101, label %sw.bb49
    i32 -102, label %sw.bb53
    i32 -103, label %sw.bb57
    i32 110, label %sw.bb61
    i32 -110, label %sw.bb65
  ]

sw.bb:                                            ; preds = %entry
  %call = call float @Dummy(i32 0)
  %add = fadd float %call, 1.000000e+00
  %conv = fpext float %add to double
  br label %return

sw.bb1:                                           ; preds = %entry
  %call2 = call float @Dummy(i32 1)
  %add3 = fadd float %call2, 2.000000e+00
  %conv4 = fpext float %add3 to double
  br label %return

sw.bb5:                                           ; preds = %entry
  %call6 = call float @Dummy(i32 2)
  %add7 = fadd float %call6, 4.000000e+00
  %conv8 = fpext float %add7 to double
  br label %return

sw.bb9:                                           ; preds = %entry
  %call10 = call float @Dummy(i32 3)
  %add11 = fadd float %call10, 8.000000e+00
  %conv12 = fpext float %add11 to double
  br label %return

sw.bb13:                                          ; preds = %entry
  %call14 = call float @Dummy(i32 -1)
  %conv15 = fpext float %call14 to double
  %add16 = fadd double %conv15, 5.000000e-01
  br label %return

sw.bb17:                                          ; preds = %entry
  %call18 = call float @Dummy(i32 -2)
  %conv19 = fpext float %call18 to double
  %add20 = fadd double %conv19, 0x3FD5555555555555
  br label %return

sw.bb21:                                          ; preds = %entry
  %call22 = call float @Dummy(i32 -3)
  %conv23 = fpext float %call22 to double
  %add24 = fadd double %conv23, 2.500000e-01
  br label %return

sw.bb25:                                          ; preds = %entry
  %call26 = call float @Dummy(i32 10)
  %conv27 = fpext float %call26 to double
  %add28 = fadd double %conv27, 0x7FF8000000000000
  br label %return

sw.bb29:                                          ; preds = %entry
  %call30 = call float @Dummy(i32 -10)
  %conv31 = fpext float %call30 to double
  %add32 = fadd double %conv31, 0xFFF8000000000000
  br label %return

sw.bb33:                                          ; preds = %entry
  %call34 = call float @Dummy(i32 100)
  %add35 = fadd float %call34, 1.000000e+00
  %conv36 = fpext float %add35 to double
  br label %return

sw.bb37:                                          ; preds = %entry
  %call38 = call float @Dummy(i32 101)
  %add39 = fadd float %call38, 2.000000e+00
  %conv40 = fpext float %add39 to double
  br label %return

sw.bb41:                                          ; preds = %entry
  %call42 = call float @Dummy(i32 102)
  %add43 = fadd float %call42, 4.000000e+00
  %conv44 = fpext float %add43 to double
  br label %return

sw.bb45:                                          ; preds = %entry
  %call46 = call float @Dummy(i32 103)
  %add47 = fadd float %call46, 8.000000e+00
  %conv48 = fpext float %add47 to double
  br label %return

sw.bb49:                                          ; preds = %entry
  %call50 = call float @Dummy(i32 -101)
  %conv51 = fpext float %call50 to double
  %add52 = fadd double %conv51, 5.000000e-01
  br label %return

sw.bb53:                                          ; preds = %entry
  %call54 = call float @Dummy(i32 -102)
  %conv55 = fpext float %call54 to double
  %add56 = fadd double %conv55, 0x3FD5555555555555
  br label %return

sw.bb57:                                          ; preds = %entry
  %call58 = call float @Dummy(i32 -103)
  %conv59 = fpext float %call58 to double
  %add60 = fadd double %conv59, 2.500000e-01
  br label %return

sw.bb61:                                          ; preds = %entry
  %call62 = call float @Dummy(i32 110)
  %conv63 = fpext float %call62 to double
  %add64 = fadd double %conv63, 0x7FF8000000000000
  br label %return

sw.bb65:                                          ; preds = %entry
  %call66 = call float @Dummy(i32 -110)
  %conv67 = fpext float %call66 to double
  %add68 = fadd double %conv67, 0xFFF8000000000000
  br label %return

return:                                           ; preds = %entry, %sw.bb65, %sw.bb61, %sw.bb57, %sw.bb53, %sw.bb49, %sw.bb45, %sw.bb41, %sw.bb37, %sw.bb33, %sw.bb29, %sw.bb25, %sw.bb21, %sw.bb17, %sw.bb13, %sw.bb9, %sw.bb5, %sw.bb1, %sw.bb
  %retval.0 = phi double [ %add68, %sw.bb65 ], [ %add64, %sw.bb61 ], [ %add60, %sw.bb57 ], [ %add56, %sw.bb53 ], [ %add52, %sw.bb49 ], [ %conv48, %sw.bb45 ], [ %conv44, %sw.bb41 ], [ %conv40, %sw.bb37 ], [ %conv36, %sw.bb33 ], [ %add32, %sw.bb29 ], [ %add28, %sw.bb25 ], [ %add24, %sw.bb21 ], [ %add20, %sw.bb17 ], [ %add16, %sw.bb13 ], [ %conv12, %sw.bb9 ], [ %conv8, %sw.bb5 ], [ %conv4, %sw.bb1 ], [ %conv, %sw.bb ], [ 0.000000e+00, %entry ]
  ret double %retval.0
}

define internal double @FpLookup4(i32 %Arg) {
entry:
  switch i32 %Arg, label %return [
    i32 0, label %sw.bb
    i32 1, label %sw.bb1
    i32 2, label %sw.bb5
    i32 3, label %sw.bb9
    i32 -1, label %sw.bb13
    i32 -2, label %sw.bb17
    i32 -3, label %sw.bb21
    i32 10, label %sw.bb25
    i32 -10, label %sw.bb29
    i32 100, label %sw.bb33
    i32 101, label %sw.bb37
    i32 102, label %sw.bb41
    i32 103, label %sw.bb45
    i32 -101, label %sw.bb49
    i32 -102, label %sw.bb53
    i32 -103, label %sw.bb57
    i32 110, label %sw.bb61
    i32 -110, label %sw.bb65
  ]

sw.bb:                                            ; preds = %entry
  %call = call float @Dummy(i32 0)
  %add = fadd float %call, 1.000000e+00
  %conv = fpext float %add to double
  br label %return

sw.bb1:                                           ; preds = %entry
  %call2 = call float @Dummy(i32 1)
  %add3 = fadd float %call2, 2.000000e+00
  %conv4 = fpext float %add3 to double
  br label %return

sw.bb5:                                           ; preds = %entry
  %call6 = call float @Dummy(i32 2)
  %add7 = fadd float %call6, 4.000000e+00
  %conv8 = fpext float %add7 to double
  br label %return

sw.bb9:                                           ; preds = %entry
  %call10 = call float @Dummy(i32 3)
  %add11 = fadd float %call10, 8.000000e+00
  %conv12 = fpext float %add11 to double
  br label %return

sw.bb13:                                          ; preds = %entry
  %call14 = call float @Dummy(i32 -1)
  %conv15 = fpext float %call14 to double
  %add16 = fadd double %conv15, 5.000000e-01
  br label %return

sw.bb17:                                          ; preds = %entry
  %call18 = call float @Dummy(i32 -2)
  %conv19 = fpext float %call18 to double
  %add20 = fadd double %conv19, 0x3FD5555555555555
  br label %return

sw.bb21:                                          ; preds = %entry
  %call22 = call float @Dummy(i32 -3)
  %conv23 = fpext float %call22 to double
  %add24 = fadd double %conv23, 2.500000e-01
  br label %return

sw.bb25:                                          ; preds = %entry
  %call26 = call float @Dummy(i32 10)
  %conv27 = fpext float %call26 to double
  %add28 = fadd double %conv27, 0x7FF8000000000000
  br label %return

sw.bb29:                                          ; preds = %entry
  %call30 = call float @Dummy(i32 -10)
  %conv31 = fpext float %call30 to double
  %add32 = fadd double %conv31, 0xFFF8000000000000
  br label %return

sw.bb33:                                          ; preds = %entry
  %call34 = call float @Dummy(i32 100)
  %add35 = fadd float %call34, 1.000000e+00
  %conv36 = fpext float %add35 to double
  br label %return

sw.bb37:                                          ; preds = %entry
  %call38 = call float @Dummy(i32 101)
  %add39 = fadd float %call38, 2.000000e+00
  %conv40 = fpext float %add39 to double
  br label %return

sw.bb41:                                          ; preds = %entry
  %call42 = call float @Dummy(i32 102)
  %add43 = fadd float %call42, 4.000000e+00
  %conv44 = fpext float %add43 to double
  br label %return

sw.bb45:                                          ; preds = %entry
  %call46 = call float @Dummy(i32 103)
  %add47 = fadd float %call46, 8.000000e+00
  %conv48 = fpext float %add47 to double
  br label %return

sw.bb49:                                          ; preds = %entry
  %call50 = call float @Dummy(i32 -101)
  %conv51 = fpext float %call50 to double
  %add52 = fadd double %conv51, 5.000000e-01
  br label %return

sw.bb53:                                          ; preds = %entry
  %call54 = call float @Dummy(i32 -102)
  %conv55 = fpext float %call54 to double
  %add56 = fadd double %conv55, 0x3FD5555555555555
  br label %return

sw.bb57:                                          ; preds = %entry
  %call58 = call float @Dummy(i32 -103)
  %conv59 = fpext float %call58 to double
  %add60 = fadd double %conv59, 2.500000e-01
  br label %return

sw.bb61:                                          ; preds = %entry
  %call62 = call float @Dummy(i32 110)
  %conv63 = fpext float %call62 to double
  %add64 = fadd double %conv63, 0x7FF8000000000000
  br label %return

sw.bb65:                                          ; preds = %entry
  %call66 = call float @Dummy(i32 -110)
  %conv67 = fpext float %call66 to double
  %add68 = fadd double %conv67, 0xFFF8000000000000
  br label %return

return:                                           ; preds = %entry, %sw.bb65, %sw.bb61, %sw.bb57, %sw.bb53, %sw.bb49, %sw.bb45, %sw.bb41, %sw.bb37, %sw.bb33, %sw.bb29, %sw.bb25, %sw.bb21, %sw.bb17, %sw.bb13, %sw.bb9, %sw.bb5, %sw.bb1, %sw.bb
  %retval.0 = phi double [ %add68, %sw.bb65 ], [ %add64, %sw.bb61 ], [ %add60, %sw.bb57 ], [ %add56, %sw.bb53 ], [ %add52, %sw.bb49 ], [ %conv48, %sw.bb45 ], [ %conv44, %sw.bb41 ], [ %conv40, %sw.bb37 ], [ %conv36, %sw.bb33 ], [ %add32, %sw.bb29 ], [ %add28, %sw.bb25 ], [ %add24, %sw.bb21 ], [ %add20, %sw.bb17 ], [ %add16, %sw.bb13 ], [ %conv12, %sw.bb9 ], [ %conv8, %sw.bb5 ], [ %conv4, %sw.bb1 ], [ %conv, %sw.bb ], [ 0.000000e+00, %entry ]
  ret double %retval.0
}

; Make sure the constants in pools are shuffled.

; Check for float pool
; X86-LABEL: .rodata.cst4
; X86: 00000041 0000c0ff 0000803f 00008040
; X86: 0000c07f 0000003f 0000803e 00000040

; Check for double pool
; X86-LABEL: .rodata.cst8
; X86: 00000000 0000f8ff 00000000 0000f87f
; X86: 00000000 0000e03f 55555555 5555d53f
; X86: 00000000 0000d03f

; X86-LABEL: .text
