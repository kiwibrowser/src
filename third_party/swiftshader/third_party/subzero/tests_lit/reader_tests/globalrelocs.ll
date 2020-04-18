; Tests if we handle global variables with relocation initializers.

; RUN: %p2i -i %s --insts | FileCheck %s
; RUN: %l2i -i %s --insts | %ifl FileCheck %s
; RUN: %lc2i -i %s --insts | %iflc FileCheck %s
; RUN:   %p2i -i %s --args -notranslate -timing | \
; RUN:   FileCheck --check-prefix=NOIR %s

@bytes = internal global [7 x i8] c"abcdefg"
; CHECK: @bytes = internal global [7 x i8] c"abcdefg"

@const_bytes = internal constant [7 x i8] c"abcdefg"
; CHECK-NEXT: @const_bytes = internal constant [7 x i8] c"abcdefg"

@ptr_to_ptr = internal global i32 ptrtoint (i32* @ptr to i32)
; CHECK-NEXT: @ptr_to_ptr = internal global i32 ptrtoint (i32* @ptr to i32)

@const_ptr_to_ptr = internal constant i32 ptrtoint (i32* @ptr to i32)
; CHECK-NEXT: @const_ptr_to_ptr = internal constant i32 ptrtoint (i32* @ptr to i32)

@ptr_to_func = internal global i32 ptrtoint (void ()* @func to i32)
; CHECK-NEXT: @ptr_to_func = internal global i32 ptrtoint (void ()* @func to i32)

@const_ptr_to_func = internal constant i32 ptrtoint (void ()* @func to i32)
; CHECK-NEXT: @const_ptr_to_func = internal constant i32 ptrtoint (void ()* @func to i32)

@compound = internal global <{ [3 x i8], i32 }> <{ [3 x i8] c"foo", i32 ptrtoint (void ()* @func to i32) }>
; CHECK-NEXT: @compound = internal global <{ [3 x i8], i32 }> <{ [3 x i8] c"foo", i32 ptrtoint (void ()* @func to i32) }>

@const_compound = internal constant <{ [3 x i8], i32 }> <{ [3 x i8] c"foo", i32 ptrtoint (void ()* @func to i32) }>

; CHECK-NEXT: @const_compound = internal constant <{ [3 x i8], i32 }> <{ [3 x i8] c"foo", i32 ptrtoint (void ()* @func to i32) }>

@ptr = internal global i32 ptrtoint ([7 x i8]* @bytes to i32)
; CHECK-NEXT: @ptr = internal global i32 ptrtoint ([7 x i8]* @bytes to i32)

@const_ptr = internal constant i32 ptrtoint ([7 x i8]* @bytes to i32)
; CHECK-NEXT: @const_ptr = internal constant i32 ptrtoint ([7 x i8]* @bytes to i32)

@addend_ptr = internal global i32 add (i32 ptrtoint (i32* @ptr to i32), i32 1)
; CHECK-NEXT: @addend_ptr = internal global i32 add (i32 ptrtoint (i32* @ptr to i32), i32 1)

@const_addend_ptr = internal constant i32 add (i32 ptrtoint (i32* @ptr to i32), i32 1)
; CHECK-NEXT: @const_addend_ptr = internal constant i32 add (i32 ptrtoint (i32* @ptr to i32), i32 1)

@addend_negative = internal global i32 add (i32 ptrtoint (i32* @ptr to i32), i32 -1)
; CHECK-NEXT: @addend_negative = internal global i32 add (i32 ptrtoint (i32* @ptr to i32), i32 -1)

@const_addend_negative = internal constant i32 add (i32 ptrtoint (i32* @ptr to i32), i32 -1)
; CHECK-NEXT: @const_addend_negative = internal constant i32 add (i32 ptrtoint (i32* @ptr to i32), i32 -1)

@addend_array1 = internal global i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 1)
; CHECK-NEXT: @addend_array1 = internal global i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 1)

@const_addend_array1 = internal constant i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 1)
; CHECK-NEXT: @const_addend_array1 = internal constant i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 1)

@addend_array2 = internal global i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 7)
; CHECK-NEXT: @addend_array2 = internal global i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 7)

@const_addend_array2 = internal constant i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 7)
; CHECK-NEXT: @const_addend_array2 = internal constant i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 7)

@addend_array3 = internal global i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 9)
; CHECK-NEXT: @addend_array3 = internal global i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 9)

@const_addend_array3 = internal constant i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 9)
; CHECK-NEXT: @const_addend_array3 = internal constant i32 add (i32 ptrtoint ([7 x i8]* @bytes to i32), i32 9)

@addend_struct1 = internal global i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 1)
; CHECK-NEXT: @addend_struct1 = internal global i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 1)

@const_addend_struct1 = internal constant i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 1)
; CHECK-NEXT: @const_addend_struct1 = internal constant i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 1)

@addend_struct2 = internal global i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 4)
; CHECK-NEXT: @addend_struct2 = internal global i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 4)

@const_addend_struct2 = internal constant i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 4)
; CHECK-NEXT: @const_addend_struct2 = internal constant i32 add (i32 ptrtoint (<{ [3 x i8], i32 }>* @compound to i32), i32 4)

@ptr_to_func_align = internal global i32 ptrtoint (void ()* @func to i32), align 8
; CHECK-NEXT: @ptr_to_func_align = internal global i32 ptrtoint (void ()* @func to i32), align 8

@const_ptr_to_func_align = internal constant i32 ptrtoint (void ()* @func to i32), align 8
; CHECK-NEXT: @const_ptr_to_func_align = internal constant i32 ptrtoint (void ()* @func to i32), align 8

@char = internal constant [1 x i8] c"0"
; CHECK-NEXT: @char = internal constant [1 x i8] c"0"

@short = internal constant [2 x i8] zeroinitializer
; CHECK-NEXT: @short = internal constant [2 x i8] zeroinitializer

define internal void @func() {
  ret void
}

; CHECK-NEXT: define internal void @func() {

; NOIR: Total across all functions
