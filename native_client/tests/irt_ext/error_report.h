/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests which want to print to standard out or standard error cannot simply
 * use printf() or fprintf(STDERR, ...). The nature of the irt_ext tests
 * involves overriding the standard library hooks so these standard print calls
 * will not make it to the console. Error reporting should be done by calling
 * irt_ext_test_print instead. This special function will bypass the hooks
 * and print to STDERR.
 */

#ifndef NATIVE_CLIENT_TESTS_IRT_EXT_ERROR_REPORT_H
#define NATIVE_CLIENT_TESTS_IRT_EXT_ERROR_REPORT_H

#include "native_client/src/include/portability.h"

void init_error_report_module(void);

void irt_ext_test_print(const char *format, ...);

#define IRT_EXT_ASSERT_MSG(bool_expr, msg) do {                               \
    if (!(bool_expr)) {                                                       \
      irt_ext_test_print("Error at line %d, %s: " #bool_expr  " is FALSE\n"   \
                         "  %s\n",                                            \
                         __LINE__, __FILE__, msg);                            \
      irt_ext_test_print("FAIL\n");                                           \
      exit(1);                                                                \
    }                                                                         \
  } while (0)

#endif /* NATIVE_CLIENT_TESTS_IRT_EXT_ERROR_REPORT_H */
