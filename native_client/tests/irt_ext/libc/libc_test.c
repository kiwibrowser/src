/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/irt_ext/basic_calls.h"
#include "native_client/tests/irt_ext/error_report.h"
#include "native_client/tests/irt_ext/file_desc.h"
#include "native_client/tests/irt_ext/libc/libc_test.h"
#include "native_client/tests/irt_ext/mem_calls.h"
#include "native_client/tests/irt_ext/threading.h"

/* Declare a macro we can later use for EXPAND_TEST_OPERATION. */
#define RUN_TESTS(TEST_NAME) errors += TEST_FUNC_NAME(TEST_NAME)();

int main(void) {
  int errors = 0;

  /* Always initialize the error reporting module first. */
  init_error_report_module();

  /* Initialize the various modules. */
  init_basic_calls_module();
  init_file_desc_module();
  init_mem_calls_module();
  init_threading_module();

  /* Run tests. */
  EXPAND_TEST_OPERATION(RUN_TESTS);

  if (errors == 0)
    return TEST_EXIT_CODE;

  irt_ext_test_print("Standard Library tests failed with %d failures.\n",
                     errors);
  return 1;
}
