; Tests that we generate an ELF container correctly when there
; is no data section.

; RUN: %p2i -i %s --filetype=obj --output %t --args -O2 \
; RUN:   && llvm-readobj -file-headers -sections -section-data \
; RUN:       -relocations -symbols %t | FileCheck %s

; RUN: %if --need=allow_dump --command %p2i -i %s --args -O2 \
; RUN:   | %if --need=allow_dump --command \
; RUN:   llvm-mc -triple=i686-nacl -filetype=obj -o - \
; RUN:   | %if --need=allow_dump --command \
; RUN:   llvm-readobj -file-headers -sections -section-data \
; RUN:       -relocations -symbols - \
; RUN:   | %if --need=allow_dump --command FileCheck %s

declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)

define internal i32 @foo(i32 %x, i32 %len) {
  %y = add i32 %x, %x
  %dst = inttoptr i32 %y to i8*
  %src = inttoptr i32 %x to i8*
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dst, i8* %src,
                                       i32 %len, i32 1, i1 false)

  ret i32 %y
}

; Test defining a non-internal function.
define void @_start(i32 %x) {
  %ignored = call i32 @foo(i32 %x, i32 4)
  ret void
}

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
; CHECK:     Index: [[SYMTAB_INDEX]]
; CHECK-NEXT: Name: .symtab
; CHECK:     Type: SHT_SYMTAB
; CHECK:     Flags [ (0x0)
; CHECK:     ]
; CHECK:     Address: 0x0
; CHECK:     Offset: 0x{{[1-9A-F][0-9A-F]*}}
; CHECK:     Size: {{[1-9][0-9]*}}
; CHECK:     Link: {{[1-9][0-9]*}}
; CHECK:     Info: {{[1-9][0-9]*}}
; CHECK:     AddressAlignment: 4
; CHECK:     EntrySize: 16
; CHECK:   }


; CHECK: Relocations [
; CHECK:   Section ({{[0-9]+}}) .rel.text {
; CHECK:     0x1E R_386_PC32 memcpy 0x0
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
; CHECK:          Name: foo
; CHECK-NEXT:     Value: 0x0
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
; CHECK:          Name: memcpy
; CHECK-NEXT:     Value: 0x0
; CHECK-NEXT:     Size: 0
; CHECK-NEXT:     Binding: Global
; CHECK-NEXT:     Type: None
; CHECK-NEXT:     Other: 0
; CHECK-NEXT:     Section: Undefined
; CHECK-NEXT:   }
; CHECK: ]
