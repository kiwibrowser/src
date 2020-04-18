/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/tests/performance/perf_test_runner.h"


class TestCatchingFault : public PerfTest {
 public:
  TestCatchingFault() {
    ASSERT_EQ(nacl_exception_set_handler(Handler), 0);
  }

  ~TestCatchingFault() {
    // Unregister handler so that we do not invoke it accidentally.
    ASSERT_EQ(nacl_exception_set_handler(NULL), 0);
  }

  virtual void run() {
    if (!setjmp(return_jmp_buf_)) {
      // Cause crash.
      for (;;)
        *(volatile int *) 0 = 0;
    }
  }

 private:
  static void Handler(struct NaClExceptionContext *context) {
    // Clear flag to allow future faults to invoke this handler.
    ASSERT_EQ(nacl_exception_clear_flag(), 0);
    longjmp(return_jmp_buf_, 1);
  }

  static jmp_buf return_jmp_buf_;
};
PERF_TEST_DECLARE(TestCatchingFault)

jmp_buf TestCatchingFault::return_jmp_buf_;
