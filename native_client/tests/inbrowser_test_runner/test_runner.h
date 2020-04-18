/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_INBROWSER_TEST_RUNNER_TEST_RUNNER_H
#define NATIVE_CLIENT_TESTS_INBROWSER_TEST_RUNNER_TEST_RUNNER_H

int RunTests(int (*test_func)(void));

int TestRunningInBrowser(void);

#endif
