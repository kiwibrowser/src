// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This test confirms the actual data types of types defined by various
// C/C++ standards. We must ensure that the actual types match on all
// platforms.

#include <setjmp.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

#include <typeinfo>

#include "native_client/tests/toolchain/utils.h"

// Checks size and alignment.
template<typename T>
void CheckSizeAndAlignOf(const char* test_type, int size, unsigned int align) {
  char error[256];
  snprintf(error, sizeof(error), "sizeof(%s)=%d", test_type, sizeof(T));
  ASSERT(sizeof(T) == size, error);
  snprintf(error, sizeof(error), "alignof(%s)=%u", test_type, __alignof__(T));
  ASSERT(__alignof__(T) == align, error);
}

// Checks type equivalency.
template<typename T, typename A>
void CheckType(const char* test_type) {
  char error[256];
  snprintf(error, sizeof(error), "typeid(%s)=%s", test_type, typeid(T).name());
  ASSERT(typeid(T) == typeid(A), error);
}

// Checks type equivalency and size and alignment.
template<typename T, typename A>
void CheckTypeAndSizeAndAlignOf(const char* test_type,
                                int size,
                                unsigned int align) {
  CheckType<T, A>(test_type);
  CheckSizeAndAlignOf<T>(test_type, size, align);
}

int main() {
  // setjmp.h
  // TODO(eaeltsin): jmp_buf must be the same on all platforms.
  // CheckSizeAndAlignOf<jmp_buf>("jmp_buf", 48, 8);  // x86-64
  // CheckSizeAndAlignOf<jmp_buf>("jmp_buf", 36, 4);  // x86-32

  // stddef.h
  CheckTypeAndSizeAndAlignOf<size_t, unsigned int>("size_t", 4, 4);
  CheckTypeAndSizeAndAlignOf<ptrdiff_t, int>("ptrdiff_t", 4, 4);
  CheckSizeAndAlignOf<wchar_t>("wchar_t", 4, 4);

  // sys/types.h
  CheckTypeAndSizeAndAlignOf<ssize_t, int>("ssize_t", 4, 4);
  // TODO(eaeltsin): add more types

  return 0;
}
