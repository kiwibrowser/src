; This is a smoke test of function reordering.

; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    | FileCheck %s --check-prefix=DEFAULTWINDOWSIZE
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    | FileCheck %s --check-prefix=DEFAULTWINDOWSIZE


; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    -reorder-functions-window-size=1 \
; RUN:    | FileCheck %s --check-prefix=WINDOWSIZE1
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    -reorder-functions-window-size=1 \
; RUN:    | FileCheck %s --check-prefix=WINDOWSIZE1


; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    -threads=0 \
; RUN:    | FileCheck %s --check-prefix=SEQUENTIAL
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    -threads=0 \
; RUN:    | FileCheck %s --check-prefix=SEQUENTIAL


; RUN: %p2i -i %s --filetype=obj --disassemble --args -O2 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    -reorder-functions-window-size=0xffffffff \
; RUN:    | FileCheck %s --check-prefix=WINDOWSIZEMAX
; RUN: %p2i -i %s --filetype=obj --disassemble --args -Om1 \
; RUN:    -sz-seed=1 -reorder-functions \
; RUN:    -reorder-functions-window-size=0xffffffff \
; RUN:    | FileCheck %s --check-prefix=WINDOWSIZEMAX

define internal void @func1() {
  ret void
}

define internal void @func2() {
  ret void
}

define internal void @func3() {
  ret void
}

define internal void @func4() {
  ret void
}

define internal void @func5() {
  ret void
}

define internal void @func6() {
  ret void
}

; DEFAULTWINDOWSIZE-LABEL: func1
; DEFAULTWINDOWSIZE-LABEL: func4
; DEFAULTWINDOWSIZE-LABEL: func5
; DEFAULTWINDOWSIZE-LABEL: func2
; DEFAULTWINDOWSIZE-LABEL: func6
; DEFAULTWINDOWSIZE-LABEL: func3

; WINDOWSIZE1-LABEL: func1
; WINDOWSIZE1-LABEL: func2
; WINDOWSIZE1-LABEL: func3
; WINDOWSIZE1-LABEL: func4
; WINDOWSIZE1-LABEL: func5
; WINDOWSIZE1-LABEL: func6

; SEQUENTIAL-LABEL: func1
; SEQUENTIAL-LABEL: func2
; SEQUENTIAL-LABEL: func3
; SEQUENTIAL-LABEL: func4
; SEQUENTIAL-LABEL: func5
; SEQUENTIAL-LABEL: func6

; WINDOWSIZEMAX-LABEL: func1
; WINDOWSIZEMAX-LABEL: func4
; WINDOWSIZEMAX-LABEL: func5
; WINDOWSIZEMAX-LABEL: func2
; WINDOWSIZEMAX-LABEL: func6
; WINDOWSIZEMAX-LABEL: func3
