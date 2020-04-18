/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_PERFORMANCE_PERF_TEST_RUNNER_H_
#define NATIVE_CLIENT_TESTS_PERFORMANCE_PERF_TEST_RUNNER_H_

class PerfTest {
 public:
  virtual ~PerfTest() {}
  virtual void run() = 0;
};

#define PERF_TEST_DECLARE(class_name) \
    PerfTest *Make##class_name() { return new class_name(); }

#endif
