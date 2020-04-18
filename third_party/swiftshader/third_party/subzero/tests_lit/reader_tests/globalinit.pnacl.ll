; Test of global initializers.

; RUN: %p2i -i %s --insts --args -allow-externally-defined-symbols \
; RUN: | FileCheck %s
; RUN: %l2i -i %s --insts --args -allow-externally-defined-symbols \
; RUN: | %ifl FileCheck %s
; RUN: %lc2i -i %s --insts --args -allow-externally-defined-symbols \
; RUN: | %iflc FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing \
; RUN:        -allow-externally-defined-symbols | \
; RUN:   FileCheck --check-prefix=NOIR %s

@PrimitiveInit = internal global [4 x i8] c"\1B\00\00\00", align 4
; CHECK: @PrimitiveInit = internal global [4 x i8] c"\1B\00\00\00", align 4

@PrimitiveInitConst = internal constant [4 x i8] c"\0D\00\00\00", align 4
; CHECK-NEXT: @PrimitiveInitConst = internal constant [4 x i8] c"\0D\00\00\00", align 4

@ArrayInit = internal global [20 x i8] c"\0A\00\00\00\14\00\00\00\1E\00\00\00(\00\00\002\00\00\00", align 4
; CHECK-NEXT: @ArrayInit = internal global [20 x i8] c"\0A\00\00\00\14\00\00\00\1E\00\00\00(\00\00\002\00\00\00", align 4

@ArrayInitPartial = internal global [40 x i8] c"<\00\00\00F\00\00\00P\00\00\00Z\00\00\00d\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00", align 4
; CHECK-NEXT: @ArrayInitPartial = internal global [40 x i8] c"<\00\00\00F\00\00\00P\00\00\00Z\00\00\00d\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00", align 4

@PrimitiveInitStatic = internal global [4 x i8] zeroinitializer, align 4
; CHECK-NEXT: @PrimitiveInitStatic = internal global [4 x i8] zeroinitializer, align 4

@PrimitiveUninit = internal global [4 x i8] zeroinitializer, align 4
; CHECK-NEXT: @PrimitiveUninit = internal global [4 x i8] zeroinitializer, align 4

@ArrayUninit = internal global [20 x i8] zeroinitializer, align 4
; CHECK-NEXT: @ArrayUninit = internal global [20 x i8] zeroinitializer, align 4

@ArrayUninitConstDouble = internal constant [200 x i8] zeroinitializer, align 8
; CHECK-NEXT: @ArrayUninitConstDouble = internal constant [200 x i8] zeroinitializer, align 8

@ArrayUninitConstInt = internal constant [20 x i8] zeroinitializer, align 4
; CHECK-NEXT: @ArrayUninitConstInt = internal constant [20 x i8] zeroinitializer, align 4

@__init_array_start = internal constant [0 x i8] zeroinitializer, align 4
; CHECK-NEXT: @__init_array_start = internal constant [0 x i8] zeroinitializer, align 4

@__fini_array_start = internal constant [0 x i8] zeroinitializer, align 4
; CHECK: @__fini_array_start = internal constant [0 x i8] zeroinitializer, align 4

@__tls_template_start = internal constant [0 x i8] zeroinitializer, align 8
; CHECK: @__tls_template_start = internal constant [0 x i8] zeroinitializer, align 8

@__tls_template_alignment = internal constant [4 x i8] c"\01\00\00\00", align 4
; CHECK: @__tls_template_alignment = internal constant [4 x i8] c"\01\00\00\00", align 4

define internal i32 @main(i32 %argc, i32 %argv) {
entry:
  %expanded1 = ptrtoint [4 x i8]* @PrimitiveInit to i32
  call void @use(i32 %expanded1)
  %expanded3 = ptrtoint [4 x i8]* @PrimitiveInitConst to i32
  call void @use(i32 %expanded3)
  %expanded5 = ptrtoint [4 x i8]* @PrimitiveInitStatic to i32
  call void @use(i32 %expanded5)
  %expanded7 = ptrtoint [4 x i8]* @PrimitiveUninit to i32
  call void @use(i32 %expanded7)
  %expanded9 = ptrtoint [20 x i8]* @ArrayInit to i32
  call void @use(i32 %expanded9)
  %expanded11 = ptrtoint [40 x i8]* @ArrayInitPartial to i32
  call void @use(i32 %expanded11)
  %expanded13 = ptrtoint [20 x i8]* @ArrayUninit to i32
  call void @use(i32 %expanded13)
  ret i32 0
}

declare void @use(i32)

define internal i32 @nacl_tp_tdb_offset(i32 %__0) {
entry:
  ret i32 0
}

define internal i32 @nacl_tp_tls_offset(i32 %size) {
entry:
  %result = sub i32 0, %size
  ret i32 %result
}

; NOIR: Total across all functions
