/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "native_client/src/include/nacl_assert.h"

/*
 * These functions would typically be declared in a shared header file, but for
 * testing convenience, they are defined here.
 */
int get_module_a_var();
int get_module_b_var();
int get_module_c_var();

/*
 * The loader should only need to load "dependencies_test", and the dependencies
 * should by loaded automatically by the pll_loader.
 *
 * | dependencies_test (depends on libc and test_pll_c)
 *   | libc
 *     * No dependencies
 *   | test_pll_c (depends on test_pll_a and test_pll_b)
 *     | test_pll_a
 *       * No dependencies
 *     | test_pll_b (depends on test_pll_a)
 *       | test_pll_a
 *         * No dependencies
 */

int main(int argc, char *argv[]) {
  ASSERT_EQ(get_module_a_var(), 2345);
  ASSERT_EQ(get_module_b_var(), 1234);
  ASSERT_EQ(get_module_c_var(), 1111);
  return 0;
}
