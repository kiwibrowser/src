/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <string.h>

#include "native_client/src/include/nacl_assert.h"


// This test checks the representation of C++ method pointers used by
// the C++ front end.

// Define example classes where a method pointer's this-offset and
// vtable-offset are both non-zero.

class TestBaseClass1 {
 public:
  virtual ~TestBaseClass1() {}
  virtual void some_virtual() {}
  char data[1000];
};

class TestBaseClass2 {
 public:
  virtual ~TestBaseClass2() {}
  int nonvirtual_method() { return 123; }
  virtual void other_method1() {}
  virtual void other_method2() {}
  virtual int virtual_method() { return 456; }
};

class TestClass : public TestBaseClass1, public TestBaseClass2 {
};

// Offset of virtual_method() in vtable: it comes after
// ~TestBaseClass1(), ~TestBaseClass2(), other_method1() and
// other_method2().
const int kVtableOffset = sizeof(void *) * 4;
const int kThisOffset = sizeof(TestBaseClass1);

// Mangled name for TestBaseClass2::method()
#define NONVIRTUAL_METHOD _ZN14TestBaseClass217nonvirtual_methodEv
extern "C" void NONVIRTUAL_METHOD();


typedef int (TestClass::*method_ptr)();

struct method_ptr_repr {
  ptrdiff_t ptr;
  ptrdiff_t adj;
};


int main() {
  ASSERT_EQ(sizeof(method_ptr), 8);
  ASSERT_EQ(kThisOffset, 1004);

  method_ptr nonvirtual_mp = &TestClass::nonvirtual_method;
  method_ptr virtual_mp = &TestClass::virtual_method;

  // Check that method pointers work.
  TestClass instance;
  ASSERT_EQ((instance.*nonvirtual_mp)(), 123);
  ASSERT_EQ((instance.*virtual_mp)(), 456);

  method_ptr_repr nonvirtual_mpr;
  method_ptr_repr virtual_mpr;
  // Copy to avoid strict-aliasing problems.
  memcpy(&nonvirtual_mpr, &nonvirtual_mp, sizeof(method_ptr));
  memcpy(&virtual_mpr, &virtual_mp, sizeof(method_ptr));

#if defined(__arm__) || defined(__pnacl__) || defined(__mips__)
  // In the ARM scheme, adj & 1 indicates whether the method is
  // virtual.  This makes no assumption about the alignment of
  // function pointers.  PNaCl uses the same scheme (see
  // https://code.google.com/p/nativeclient/issues/detail?id=3450).

  ASSERT_EQ(nonvirtual_mpr.ptr, (ptrdiff_t) NONVIRTUAL_METHOD);
  ASSERT_EQ(nonvirtual_mpr.adj, kThisOffset << 1);

  ASSERT_EQ(virtual_mpr.ptr, kVtableOffset);
  ASSERT_EQ(virtual_mpr.adj, (kThisOffset << 1) + 1);
#else
  // In the Itanium scheme (used for x86), ptr & 1 indicates whether
  // the method is virtual.  This assumes that function pointers are 0
  // mod 2, so we do not want to use this scheme for PNaCl.

  ASSERT_EQ(nonvirtual_mpr.ptr, (ptrdiff_t) NONVIRTUAL_METHOD);
  ASSERT_EQ(nonvirtual_mpr.adj, kThisOffset);

  ASSERT_EQ(virtual_mpr.ptr, kVtableOffset + 1);
  ASSERT_EQ(virtual_mpr.adj, kThisOffset);
#endif

  return 0;
}
