/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/tests/syscalls/test.h"

#include <cstdio>

bool test::Failed(const char *testname,
            const char *msg,
            const char* file_name,
            int line) {
  if (0 == file_name) {
    file_name = "UNKNOWN";
  }
  std::printf("TEST FAILED: %s: %s at %s:%d\n",
              testname,
              msg,
              file_name,
              line);
  return false;
}

bool test::Passed(const char *testname, const char *msg) {
  printf("TEST PASSED: %s: %s\n", testname, msg);
  return true;
}
