/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_IRT_EXT_LIBC_TEST_H
#define NATIVE_CLIENT_TESTS_IRT_EXT_LIBC_TEST_H

#include "native_client/src/include/nacl_macros.h"
#include "native_client/tests/irt_ext/error_report.h"

/*
 * This macro is used to expand macros for declaring and running the test
 * functions. It also serves as a central location where all the tests are
 * listed out. The test names must match the test name when defining tests
 * using the "DEFINE_TEST" macro below.
 *
 * For example, having listed OP(Basic), we expect to have in another
 * compiled .c file somewhere DEFINE_TEST(Basic, ...).
 */
#define EXPAND_TEST_OPERATION(OP) \
  OP(Basic);                      \
  OP(File);                       \
  OP(Mem);                        \
  OP(Thread);

/* This macro specifies the format of our test function name. */
#define TEST_FUNC_NAME(TEST_NAME) run_##TEST_NAME##_tests

/* This macro is used to define the test body. */
#define DEFINE_TEST(TEST_NAME, TEST_FUNCS_ARRAY, ENV_TYPE, \
                    SETUP_FUNC, TEARDOWN_FUNC) \
  int TEST_FUNC_NAME(TEST_NAME)(void) { \
    ENV_TYPE env_desc; \
    int num_success = 0; \
    irt_ext_test_print("Running %d " #TEST_NAME " Tests...\n", \
                       NACL_ARRAY_SIZE(TEST_FUNCS_ARRAY)); \
    for (int i = 0; i < NACL_ARRAY_SIZE(TEST_FUNCS_ARRAY); i++) { \
      SETUP_FUNC(&env_desc); \
      if (0 == TEST_FUNCS_ARRAY[i](&env_desc)) { \
        num_success++; \
      } \
      TEARDOWN_FUNC(); \
    } \
    irt_ext_test_print(#TEST_NAME " Tests results - %d/%d succeeded.\n", \
                       num_success, NACL_ARRAY_SIZE(TEST_FUNCS_ARRAY)); \
    if (num_success < NACL_ARRAY_SIZE(TEST_FUNCS_ARRAY)) { \
      irt_ext_test_print(#TEST_NAME " Test has Failed.\n"); \
    } \
    return NACL_ARRAY_SIZE(TEST_FUNCS_ARRAY) - num_success; \
  }

/* The actual declaration of the test functions. */
#define TEST_DECLARATION(TEST_NAME) int TEST_FUNC_NAME(TEST_NAME)(void)
EXPAND_TEST_OPERATION(TEST_DECLARATION)

#endif /* NATIVE_CLIENT_TESTS_IRT_EXT_LIBC_TEST_H */
