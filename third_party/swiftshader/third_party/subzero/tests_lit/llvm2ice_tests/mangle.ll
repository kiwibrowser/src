; Tests the Subzero "name mangling" when using the "pnacl-sz --prefix"
; option.  Also does a quick smoke test of -ffunction-sections.

; REQUIRES: allow_dump
; RUN: %p2i -i %s --args --verbose none -ffunction-sections | FileCheck %s
; TODO(stichnot): The following line causes this test to fail.
; RUIN: %p2i --assemble --disassemble -i %s --args --verbose none \
; RUIN:   | FileCheck %s
; RUN: %p2i -i %s --args --verbose none --prefix Subzero -ffunction-sections \
; RUN:   | FileCheck --check-prefix=MANGLE %s

define internal void @FuncC(i32 %i) {
entry:
  ret void
}
; FuncC is a C symbol that isn't recognized as a C++ mangled symbol.
; CHECK-LABEL: .text.FuncC
; CHECK: FuncC:
; MANGLE-LABEL: .text.SubzeroFuncC
; MANGLE: SubzeroFuncC:

define internal void @_ZN13TestNamespace4FuncEi(i32 %i) {
entry:
  ret void
}
; This is Func(int) nested inside namespace TestNamespace.
; CHECK-LABEL: .text._ZN13TestNamespace4FuncEi
; CHECK: _ZN13TestNamespace4FuncEi:
; MANGLE-LABEL: .text._ZN7Subzero13TestNamespace4FuncEi
; MANGLE: _ZN7Subzero13TestNamespace4FuncEi:

define internal void @_ZN13TestNamespace15NestedNamespace4FuncEi(i32 %i) {
entry:
  ret void
}
; This is Func(int) nested inside two namespaces.
; CHECK-LABEL: .text._ZN13TestNamespace15NestedNamespace4FuncEi
; CHECK: _ZN13TestNamespace15NestedNamespace4FuncEi:
; MANGLE-LABEL: .text._ZN7Subzero13TestNamespace15NestedNamespace4FuncEi
; MANGLE: _ZN7Subzero13TestNamespace15NestedNamespace4FuncEi:

define internal void @_Z13FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; This is a non-nested, mangled C++ symbol.
; CHECK-LABEL: .text._Z13FuncCPlusPlusi
; CHECK: _Z13FuncCPlusPlusi:
; MANGLE-LABEL: .text._ZN7Subzero13FuncCPlusPlusEi
; MANGLE: _ZN7Subzero13FuncCPlusPlusEi:

define internal void @_ZN12_GLOBAL__N_18FuncAnonEi(i32 %i) {
entry:
  ret void
}
; This is FuncAnon(int) nested inside an anonymous namespace.
; CHECK-LABEL: .text._ZN12_GLOBAL__N_18FuncAnonEi
; CHECK: _ZN12_GLOBAL__N_18FuncAnonEi:
; MANGLE-LABEL: .text._ZN7Subzero12_GLOBAL__N_18FuncAnonEi
; MANGLE: _ZN7Subzero12_GLOBAL__N_18FuncAnonEi:

; Now for the illegitimate examples.

; Test for _ZN with no suffix.  Don't crash, prepend Subzero.
define internal void @_ZN(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text.Subzero_ZN
; MANGLE: Subzero_ZN:

; Test for _Z<len><str> where <len> is smaller than it should be.
define internal void @_Z12FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text._ZN7Subzero12FuncCPlusPluEsi
; MANGLE: _ZN7Subzero12FuncCPlusPluEsi:

; Test for _Z<len><str> where <len> is slightly larger than it should be.
define internal void @_Z14FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text._ZN7Subzero14FuncCPlusPlusiE
; MANGLE: _ZN7Subzero14FuncCPlusPlusiE:

; Test for _Z<len><str> where <len> is much larger than it should be.
define internal void @_Z114FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text.Subzero_Z114FuncCPlusPlusi
; MANGLE: Subzero_Z114FuncCPlusPlusi:

; Test for _Z<len><str> where we try to overflow the uint32_t holding <len>.
define internal void @_Z4294967296FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text.Subzero_Z4294967296FuncCPlusPlusi
; MANGLE: Subzero_Z4294967296FuncCPlusPlusi:

; Test for _Z<len><str> where <len> is 0.
define internal void @_Z0FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text._ZN7Subzero0EFuncCPlusPlusi
; MANGLE: _ZN7Subzero0EFuncCPlusPlusi:

; Test for _Z<len><str> where <len> is -1.  LLVM explicitly allows the
; '-' character in identifiers.

define internal void @_Z-1FuncCPlusPlusi(i32 %i) {
entry:
  ret void
}
; MANGLE-LABEL: .text.Subzero_Z-1FuncCPlusPlusi
; MANGLE: Subzero_Z-1FuncCPlusPlusi:


; Test for substitution incrementing.  This single test captures:
;   S<num>_ ==> S<num+1>_ for single-digit <num>
;   S_ ==> S0_
;   String length increase, e.g. SZZZ_ ==> S1000_
;   At least one digit wrapping without length increase, e.g. SZ9ZZ_ ==> SZA00_
;   Unrelated identifiers containing S[0-9A-Z]* , e.g. MyClassS1x
;   A proper substring of S<num>_ at the end of the string
;     (to test parser edge cases)

define internal void @_Z3fooP10MyClassS1xP10MyClassS2xRS_RS1_S_S1_SZZZ_SZ9ZZ_S12345() {
; MANGLE-LABEL: .text._ZN7Subzero3fooEP10MyClassS1xP10MyClassS2xRS0_RS2_S0_S2_S1000_SZA00_S12345
; MANGLE: _ZN7Subzero3fooEP10MyClassS1xP10MyClassS2xRS0_RS2_S0_S2_S1000_SZA00_S12345:
entry:
  ret void
}

; Test that unmangled (non-C++) strings don't have substitutions updated.
define internal void @foo_S_S0_SZ_S() {
; MANGLE-LABEL: .text.Subzerofoo_S_S0_SZ_S
; MANGLE: Subzerofoo_S_S0_SZ_S:
entry:
  ret void
}
