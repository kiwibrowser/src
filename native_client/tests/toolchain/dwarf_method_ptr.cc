/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test that dwarf info for method pointers is preserved after linking.
 *
 */

class TestClass {
 public:
  int foo() { return 42; }
};

typedef int (TestClass::*method_ptr)();

__attribute__((noinline)) static int foo(TestClass *dwarf_test_method_ptr_param,
                                         method_ptr *mp) {
  // CHECK-DAG: (DW_TAG_formal_parameter)
  // CHECK-DAG: DW_AT_location
  // CHECK: DW_AT_name{{.*}} dwarf_test_method_ptr_param
  // CHECK-NEXT: {{.*}}DW_AT_decl_file{{.*}} : 1
  // CHECK-NEXT: {{.*}}DW_AT_decl_line{{.*}} : 18
  return (dwarf_test_method_ptr_param->**mp)();
}

int caller(TestClass *my_test_param) {
  method_ptr mp = &TestClass::foo;
  return foo(my_test_param, &mp);
}

int main() {
  TestClass TC;
  int ret = caller(&TC);
  return ret;
}
