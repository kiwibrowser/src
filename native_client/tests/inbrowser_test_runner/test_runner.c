/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "native_client/tests/inbrowser_test_runner/test_runner.h"


int RunTests(int (*test_func)(void)) {
  /* Turn off stdout buffering to aid debugging in case of a crash. */
  setvbuf(stdout, NULL, _IONBF, 0);
  return test_func();
}

int TestRunningInBrowser(void) {
  return 0;
}
