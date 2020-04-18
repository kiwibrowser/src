; Tests that we generate an ELF container with fields that make sense,
; cross-validating against llvm-mc.

; For the integrated ELF writer, we can't pipe the output because we need
; to seek backward and patch up the file headers. So, use a temporary file.
; RUN: %p2i -i %s --filetype=obj --output %t --args -O2 --verbose none \
; RUN:   -allow-externally-defined-symbols \
; RUN:   && llvm-readobj -file-headers -sections -section-data \
; RUN:       -relocations -symbols %t | FileCheck %s

; RUN: %if --need=allow_dump --command %p2i -i %s --args -O2 --verbose none \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | %if --need=allow_dump --command llvm-mc -triple=i686-nacl \
; RUN:     -filetype=obj -o - \
; RUN:   | %if --need=allow_dump --command llvm-readobj -file-headers \
; RUN:     -sections -section-data -relocations -symbols - \
; RUN:   | %if --need=allow_dump --command FileCheck %s

; Add a run that shows relocations in code inline.
; RUN: %p2i -i %s --filetype=obj --output %t --disassemble --args -O2 \
; RUN:   -allow-externally-defined-symbols \
; RUN:   | FileCheck --check-prefix=TEXT-RELOCS %s

; Use intrinsics to test external calls.
declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)
declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i32, i1)

; Try other external functions (for cross tests).
; Not testing external global variables since the NaCl bitcode writer
; refuses to freeze such IR.
declare void @external_foo(i32)

; Test some global data relocs (data, rodata, bss).
@bytes = internal global [7 x i8] c"ab\03\FF\F6fg", align 1
@bytes_const = internal constant [7 x i8] c"ab\03\FF\F6fg", align 1

@ptr = internal global i32 ptrtoint ([7 x i8]* @bytes to i32), align 16
@ptr_const = internal constant i32 ptrtoint ([7 x i8]* @bytes to i32), align 16

@ptr_to_func = internal global i32 ptrtoint (double ()* @returnDoubleConst to i32), align 4
@ptr_to_func_const = internal constant i32 ptrtoint (double ()* @returnDoubleConst to i32), align 4

@addend_ptr = internal global i32 add (i32 ptrtoint (i32* @ptr to i32), i32 128), align 4
@addend_ptr_const = internal constant i32 add (i32 ptrtoint (i32* @ptr to i32), i32 64), align 4

@short_zero = internal global [2 x i8] zeroinitializer, align 2
@double_zero = internal global [8 x i8] zeroinitializer, align 32
@double_zero2 = internal global [8 x i8] zeroinitializer, align 8
@short_zero_const = internal constant [2 x i8] zeroinitializer, align 2
@double_zero_const = internal constant [8 x i8] zeroinitializer, align 32
@double_zero_const2 = internal constant [8 x i8] zeroinitializer, align 8

; Use float/double constants to test constant pools.
define internal float @returnFloatConst() {
entry:
  %f = fadd float -0.0, 0x3FF3AE1400000000
  ret float %f
}
; TEXT-RELOCS-LABEL: returnFloatConst
; TEXT-RELOCS: movss {{.*}} R_386_32 {{.*}}80000000
; TEXT-RELOCS: addss {{.*}} R_386_32 {{.*}}3f9d70a0

define internal double @returnDoubleConst() {
entry:
  %d = fadd double 0x7FFFFFFFFFFFFFFFF, 0xFFF7FFFFFFFFFFFF
  %d2 = fadd double %d, 0xFFF8000000000003
  ret double %d2
}
; TEXT-RELOCS-LABEL: returnDoubleConst
; TEXT-RELOCS: movsd {{.*}} R_386_32 {{.*}}ffffffffffffffff
; TEXT-RELOCS: addsd {{.*}} R_386_32 {{.*}}fff7ffffffffffff
; TEXT-RELOCS: addsd {{.*}} R_386_32 {{.*}}fff8000000000003

; Test intrinsics that call out to external functions.
define internal void @test_memcpy(i32 %iptr_dst, i32 %len) {
entry:
  %dst = inttoptr i32 %iptr_dst to i8*
  %src = bitcast [7 x i8]* @bytes to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 %len, i32 1, i1 false)
  ret void
}
; TEXT-RELOCS-LABEL: test_memcpy
; TEXT-RELOCS: mov {{.*}} R_386_32 {{bytes|.data}}

define internal void @test_memset(i32 %iptr_dst, i32 %wide_val, i32 %len) {
entry:
  %val = trunc i32 %wide_val to i8
  %dst = inttoptr i32 %iptr_dst to i8*
  call void @llvm.memset.p0i8.i32(i8* %dst, i8 %val,
                                  i32 %len, i32 1, i1 false)
  ret void
}
; TEXT-RELOCS-LABEL: test_memset

; Test calling internal functions (may be able to do the fixup,
; without emitting a relocation).
define internal float @test_call_internal() {
  %f = call float @returnFloatConst()
  ret float %f
}

; Test calling an external function.
define internal void @test_call_external() {
  call void @external_foo(i32 42)
  ret void
}

; Test copying a function pointer, or a global data pointer.
define internal i32 @test_ret_fp() {
  %r = ptrtoint float ()* @returnFloatConst to i32
  ret i32 %r
}
; TEXT-RELOCS-LABEL: test_ret_fp
; TEXT-RELOCS-NEXT: mov {{.*}} R_386_32 {{returnFloatConst|.text}}

define internal i32 @test_ret_global_pointer() {
  %r = ptrtoint [7 x i8]* @bytes to i32
  ret i32 %r
}
; TEXT-RELOCS-LABEL: test_ret_global_pointer
; TEXT-RELOCS-NEXT: mov {{.*}} R_386_32 {{bytes|.data}}

; Test defining a non-internal function.
define void @_start(i32) {
  %f = call float @returnFloatConst()
  %d = call double @returnDoubleConst()
  call void @test_memcpy(i32 0, i32 99)
  call void @test_memset(i32 0, i32 0, i32 99)
  %f2 = call float @test_call_internal()
  %p1 = call i32 @test_ret_fp()
  %p2 = call i32 @test_ret_global_pointer()
  ret void
}

; CHECK: ElfHeader {
; CHECK:   Ident {
; CHECK:     Magic: (7F 45 4C 46)
; CHECK:     Class: 32-bit
; CHECK:     DataEncoding: LittleEndian
; CHECK:     OS/ABI: SystemV (0x0)
; CHECK:     ABIVersion: 0
; CHECK:     Unused: (00 00 00 00 00 00 00)
; CHECK:   }
; CHECK:   Type: Relocatable (0x1)
; CHECK:   Machine: EM_386 (0x3)
; CHECK:   Version: 1
; CHECK:   Entry: 0x0
; CHECK:   ProgramHeaderOffset: 0x0
; CHECK:   SectionHeaderOffset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:   Flags [ (0x0)
; CHECK:   ]
; CHECK:   HeaderSize: 52
; CHECK:   ProgramHeaderEntrySize: 0
; CHECK:   ProgramHeaderCount: 0
; CHECK:   SectionHeaderEntrySize: 40
; CHECK:   SectionHeaderCount: {{[1-9][0-9]*}}
; CHECK:   StringTableSectionIndex: {{[1-9][0-9]*}}
; CHECK: }


; CHECK: Sections [
; CHECK:   Section {
; CHECK:     Index: 0
; CHECK:     Name: (0)
; CHECK:     Type: SHT_NULL
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x0
; CHECK:     Size: 0
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 0
; CHECK:     EntrySize: 0
; CHECK:     SectionData (
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .text
; CHECK:     Type: SHT_PROGBITS
; CHECK:     Flags [ (0x6)
; CHECK:       SHF_ALLOC
; CHECK:       SHF_EXECINSTR
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 32
; CHECK:     EntrySize: 0
; CHECK:     SectionData (
;   There's probably halt padding (0xF4) in there somewhere.
; CHECK:       {{.*}}F4
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .rel.text
; CHECK:     Type: SHT_REL
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: [[SYMTAB_INDEX:[1-9][0-9]*]]
; CHECK:     Info: {{[1-9][0-9]*}}
; CHECK:     AddressAlignment: 4
; CHECK:     EntrySize: 8
; CHECK:     SectionData (
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: [[DATA_INDEX:[1-9][0-9]*]]
; CHECK:     Name: .data
; CHECK:     Type: SHT_PROGBITS
; CHECK:     Flags [ (0x3)
; CHECK:       SHF_ALLOC
; CHECK:       SHF_WRITE
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: 28
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 16
; CHECK:     EntrySize: 0
; CHECK:     SectionData (
; CHECK:       0000: 616203FF F66667{{.*}} |ab...fg
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .rel.data
; CHECK:     Type: SHT_REL
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: 24
; CHECK:     Link: [[SYMTAB_INDEX]]
; CHECK:     Info: [[DATA_INDEX]]
; CHECK:     AddressAlignment: 4
; CHECK:     EntrySize: 8
; CHECK:     SectionData (
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .bss
; CHECK:     Type: SHT_NOBITS
; CHECK:     Flags [ (0x3)
; CHECK:       SHF_ALLOC
; CHECK:       SHF_WRITE
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: 48
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 32
; CHECK:     EntrySize: 0
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .rodata
; CHECK:     Type: SHT_PROGBITS
; CHECK:     Flags [ (0x2)
; CHECK:       SHF_ALLOC
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: 48
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 32
; CHECK:     EntrySize: 0
; CHECK:     SectionData (
; CHECK:       0000: 616203FF F66667{{.*}} |ab...fg
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .rel.rodata
; CHECK:     Type: SHT_REL
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: [[SYMTAB_INDEX]]
; CHECK:     Info: {{[1-9][0-9]*}}
; CHECK:     AddressAlignment: 4
; CHECK:     EntrySize: 8
; CHECK:     SectionData (
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .rodata.cst4
; CHECK:     Type: SHT_PROGBITS
; CHECK:     Flags [ (0x12)
; CHECK:       SHF_ALLOC
; CHECK:       SHF_MERGE
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: 8
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 4
; CHECK:     EntrySize: 4
; CHECK:     SectionData (
; CHECK:       0000: A0709D3F 00000080
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .rodata.cst8
; CHECK:     Type: SHT_PROGBITS
; CHECK:     Flags [ (0x12)
; CHECK:       SHF_ALLOC
; CHECK:       SHF_MERGE
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: 24
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 8
; CHECK:     EntrySize: 8
; CHECK:     SectionData (
; CHECK:       0000: FFFFFFFF FFFFF7FF 03000000 0000F8FF
; CHECK:       0010: FFFFFFFF FFFFFFFF
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: {{[1-9][0-9]*}}
; CHECK:     Name: .shstrtab
; CHECK:     Type: SHT_STRTAB
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 1
; CHECK:     EntrySize: 0
; CHECK:     SectionData (
; CHECK:       {{.*}}.text{{.*}}
; CHECK:     )
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: [[SYMTAB_INDEX]]
; CHECK-NEXT: Name: .symtab
; CHECK:     Type: SHT_SYMTAB
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: [[STRTAB_INDEX:[1-9][0-9]*]]
; CHECK:     Info: [[GLOBAL_START_INDEX:[1-9][0-9]*]]
; CHECK:     AddressAlignment: 4
; CHECK:     EntrySize: 16
; CHECK:   }
; CHECK:   Section {
; CHECK:     Index: [[STRTAB_INDEX]]
; CHECK-NEXT: Name: .strtab
; CHECK:     Type: SHT_STRTAB
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: 0
; CHECK:     Info: 0
; CHECK:     AddressAlignment: 1
; CHECK:     EntrySize: 0
; CHECK:   }


; CHECK: Relocations [
; CHECK:   Section ({{[0-9]+}}) .rel.text {
; CHECK:     0x7 R_386_32 {{.*}}80000000 0x0
; CHECK:     0xF R_386_32 {{.*}}3f9d70a0 0x0
; CHECK:     0x27 R_386_32 {{.*}}ffffffffffffffff 0x0
; CHECK:     0x2F R_386_32 {{.*}}fff7ffffffffffff 0x0
; CHECK:     0x37 R_386_32 {{.*}}fff8000000000003 0x0
; CHECK:     0x{{.*}} R_386_PC32 memcpy
; CHECK:     0x{{.*}} R_386_PC32 memset
; CHECK:     0x{{.*}} R_386_PC32 external_foo
; CHECK:   }
; CHECK:   Section ({{[0-9]+}}) .rel.data {
; The set of relocations between llvm-mc and the integrated elf-writer
; are different. For local symbols, llvm-mc uses the section + offset within
; the section, while the integrated elf-writer refers the symbol itself.
; CHECK:     0x10 R_386_32 {{.*}} 0x0
; CHECK:     0x14 R_386_32 {{.*}} 0x0
; CHECK:     0x18 R_386_32 {{.*}} 0x0
; CHECK:   }
; CHECK:   Section ({{[0-9]+}}) .rel.rodata {
; CHECK:     0x10 R_386_32 {{.*}} 0x0
; CHECK:     0x14 R_386_32 {{.*}} 0x0
; CHECK:     0x18 R_386_32 {{.*}} 0x0
; CHECK:   }
; CHECK: ]


; CHECK: Symbols [
; CHECK-NEXT:   Symbol {
; CHECK-NEXT:     Name: (0)
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: Undefined (0x0)
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: {{.*}}fff8000000000003
; CHECK-NEXT:     Value: 0x8
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: None (0x0)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata.cst8
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: {{.*}}ffffffffffffffff
; CHECK-NEXT:     Value: 0x10
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: None (0x0)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata.cst8
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: {{.*}}3f9d70a0
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: None (0x0)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata.cst4
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: {{.*}}80000000
; CHECK-NEXT:     Value: 0x4
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: None (0x0)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata.cst4
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: addend_ptr
; CHECK-NEXT:     Value: 0x18
; CHECK-NEXT:     Size: 4
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: Object (0x1)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .data
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: addend_ptr_const
; CHECK-NEXT:     Value: 0x18
; CHECK-NEXT:     Size: 4
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: Object (0x1)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: bytes
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 7
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: Object (0x1)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .data
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: bytes_const
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 7
; CHECK-NEXT:     Binding: Local (0x0)
; CHECK-NEXT:     Type: Object (0x1)
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: double_zero
; CHECK-NEXT:     Value: 0x20
; CHECK-NEXT:     Size: 8
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .bss
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: double_zero2
; CHECK-NEXT:     Value: 0x28
; CHECK-NEXT:     Size: 8
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .bss
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: double_zero_const
; CHECK-NEXT:     Value: 0x20
; CHECK-NEXT:     Size: 8
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: double_zero_const2
; CHECK-NEXT:     Value: 0x28
; CHECK-NEXT:     Size: 8
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: ptr
; CHECK-NEXT:     Value: 0x10
; CHECK-NEXT:     Size: 4
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .data
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: ptr_const
; CHECK-NEXT:     Value: 0x10
; CHECK-NEXT:     Size: 4
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: ptr_to_func
; CHECK-NEXT:     Value: 0x14
; CHECK-NEXT:     Size: 4
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .data
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: ptr_to_func_const
; CHECK-NEXT:     Value: 0x14
; CHECK-NEXT:     Size: 4
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: returnDoubleConst
; CHECK-NEXT:     Value: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .text
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: returnFloatConst
;  This happens to be the first function, so its offset is 0 within the text.
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .text
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: short_zero
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 2
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .bss
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: short_zero_const
; CHECK-NEXT:     Value: 0x1C
; CHECK-NEXT:     Size: 2
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: Object
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .rodata
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: test_memcpy
; CHECK-NEXT:     Value: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Local
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .text
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: _start
; CHECK-NEXT:     Value: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Global
; CHECK-NEXT:     Type: Function
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: .text
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: external_foo
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Global
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: Undefined
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: memcpy
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Global
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: Undefined
; CHECK-NEXT:   }
; CHECK:        Symbol {
; CHECK:          Name: memset
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Global
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: Undefined
; CHECK-NEXT:   }
; CHECK: ]
