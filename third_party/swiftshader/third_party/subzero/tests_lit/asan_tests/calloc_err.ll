; Test that errors in source files are transparently reported by
; sz-clang.py and sz-clang++.py

; RUN: not %S/../../pydir/sz-clang.py -fsanitize-address %S/Input/calloc_err.c \
; RUN:     2>&1 | FileCheck %s

; RUN: not %S/../../pydir/sz-clang\+\+.py -fsanitize-address \
; RUN:     %S/Input/calloc_err.c 2>&1 | FileCheck %s

; CHECK-LABEL: Input/calloc_err.c:1:18: warning:
; CHECK-SAME: implicit declaration of function 'not_defined' is invalid in C99
