; Test that sz-clang.py and sz-clang++.py successfully replace calls to calloc

; RUN: %S/../../pydir/sz-clang.py -fsanitize-address %S/Input/calloc.c -E \
; RUN:     | FileCheck %s

; RUN: %S/../../pydir/sz-clang++.py -fsanitize-address %S/Input/calloc.c -E \
; RUN:     | FileCheck %s

; CHECK-LABEL: int main(void) {
; CHECK-NEXT:    void *buf = (__asan_dummy_calloc(14, sizeof(int)));
; CHECK-NEXT:    strcpy(buf, "Hello, world!");
; CHECK-NEXT:    printf("%s\n", buf);
; CHECK-NEXT:    free(buf);
; CHECK-NEXT:  }
