/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_UTILS_H_
#define NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_UTILS_H_

#include <stdio.h>
#include <string.h>

#define CHECK_EQ(x1, x2, fmt, err_ctr)                            \
  fprintf(stderr, "Checking (" fmt ") == (" fmt ")\n", x1, x2);   \
  if (x1 != x2) {                                                 \
    fprintf(stderr, "(" fmt ") != (" fmt ")\n", x1, x2);          \
    err_ctr++;                                                    \
  }

#define CHECK_STR_EQ(s1, s2, err_ctr)                                   \
  fprintf(stderr, "Checking (%s) == (%s)\n", s1, s2);                   \
  if (strcmp(s1, s2) != 0) {                                            \
    fprintf(stderr, "(%s) != expected (%s)\n",                          \
            s1, s2);                                                    \
    err_ctr++;                                                          \
  }


/* Toggle some linkage attributes to get variants of a test from the
 * same skeleton.
 */

#ifdef TEST_TLS
#define TLS_OR_NOT __thread
#else
#define TLS_OR_NOT
#endif


#endif  /* #ifndef NATIVE_CLIENT_TESTS_PNACL_SHARED_LIB_TEST_UTILS_H_ */
