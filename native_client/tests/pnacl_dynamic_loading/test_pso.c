/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/tests/pnacl_dynamic_loading/test_pso.h"


static int var = 2345;

/* Zero-initialized variables go in the BSS.  Test a large BSS. */
static char bss_var[BSS_VAR_SIZE];

static int example_func(int *ptr) {
  return *ptr + 1234;
}

static int *get_var(void) {
  /* Test use of -fPIC by getting an address. */
  return &var;
}

/*
 * Test use of LLVM's memcpy intrinsic inside a PSO.  (Clang will compile
 * calls to memcpy() to uses of LLVM's memcpy intrinsic.)
 */
static void *memcpy_example(void *dest, const void *src, size_t size) {
  return memcpy(dest, src, size);
}

/*
 * Test use of 64-bit division.  For a 32-bit architecture, this will call
 * a function such as __divdi3, so this tests that such a function gets
 * linked in if needed.
 */
static int64_t division_example(int64_t a, int64_t b) {
  return a / b;
}

struct test_pso_root __pnacl_pso_root = {
  example_func,
  get_var,
  bss_var,
  memcpy_example,
  division_example,
};
