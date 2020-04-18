/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime, assertion macros.
 *
 * THIS FILE SHOULD BE USED ONLY IN TEST CODE.
 */
#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_ASSERT_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_ASSERT_H_

#include <stdio.h>

#include "native_client/src/include/portability.h"
/* get NACL_PRIxPTR */

/*
 * Instead of using <assert.h>, we define a version that works with
 * our testing framework, printing out FAIL / SUCCESS to standard
 * output and then exiting with a non-zero exit status.
 */

#define ASSERT(bool_expr) do {                                            \
    if (!(bool_expr)) {                                                   \
      fprintf(stderr,                                                     \
              "Error at line %d, %s: " #bool_expr  " is FALSE\n",         \
              __LINE__, __FILE__);                                        \
      printf("FAIL\n");                                                   \
      exit(1);                                                            \
    }                                                                     \
  } while (0)

#define ASSERT_MSG(bool_expr, msg) do {                                   \
    if (!(bool_expr)) {                                                   \
      fprintf(stderr,                                                     \
              "Error at line %d, %s: " #bool_expr  " is FALSE\n",         \
              __LINE__, __FILE__);                                        \
      fprintf(stderr, "%s\n", msg);                                       \
      printf("FAIL\n");                                                   \
      exit(1);                                                            \
    }                                                                     \
  } while (0)

static INLINE void NaClAssertOpFailMessage(uintptr_t lhs,
                                           uintptr_t rhs,
                                           const char *lhs_expr,
                                           const char *rhs_expr,
                                           const char *comparison_op,
                                           int source_line,
                                           const char *source_file) {
  fprintf(stderr,
          "Error at line %d, %s:\n"
          "Error: %s %s %s is FALSE\n",
          source_line, source_file, lhs_expr, comparison_op, rhs_expr);
  fprintf(stderr,
          "got 0x%08" NACL_PRIxPTR " (%" NACL_PRIdPTR "); "
          "comparison value 0x%08" NACL_PRIxPTR " (%" NACL_PRIdPTR ")\n",
          lhs, lhs, rhs, rhs);
}

#define ASSERT_OP_THUNK(lhs, op, rhs, slhs, srhs, thunk) do {             \
    if (!((lhs) op (rhs))) {                                              \
      NaClAssertOpFailMessage((uintptr_t) (lhs), (uintptr_t) (rhs),       \
                              slhs, srhs, #op, __LINE__, __FILE__);       \
      thunk;                                                              \
      printf("FAIL\n");                                                   \
      exit(1);                                                            \
    }                                                                     \
  } while (0)

#define ASSERT_OP_MSG(lhs, op, rhs, slhs, srhs, msg)                      \
  ASSERT_OP_THUNK(lhs, op, rhs,                                           \
                  slhs, srhs, do { fprintf(stderr, "%s\n", msg); } while (0))

#define ASSERT_EQ_MSG(lhs, rhs, msg)            \
  ASSERT_OP_MSG(lhs, ==, rhs, #lhs, #rhs, msg)
#define ASSERT_NE_MSG(lhs, rhs, msg)            \
  ASSERT_OP_MSG(lhs, !=, rhs, #lhs, #rhs, msg)
#define ASSERT_LE_MSG(lhs, rhs, msg)            \
  ASSERT_OP_MSG(lhs, <=, rhs, #lhs, #rhs, msg)

#define ASSERT_OP(lhs, op, rhs, slhs, srhs)                     \
  ASSERT_OP_THUNK(lhs, op, rhs, slhs, srhs, do { ; } while (0))

#define ASSERT_EQ(lhs, rhs) ASSERT_OP(lhs, ==, rhs, #lhs, #rhs)
#define ASSERT_NE(lhs, rhs) ASSERT_OP(lhs, !=, rhs, #lhs, #rhs)
#define ASSERT_LE(lhs, rhs) ASSERT_OP(lhs, <=, rhs, #lhs, #rhs)
#define ASSERT_GE(lhs, rhs) ASSERT_OP(lhs, >=, rhs, #lhs, #rhs)
#define ASSERT_LT(lhs, rhs) ASSERT_OP(lhs, <, rhs, #lhs, #rhs)
#define ASSERT_GT(lhs, rhs) ASSERT_OP(lhs, >, rhs, #lhs, #rhs)

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_ASSERT_H_ */
