// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The test log collector uses Event Tracing for Windows to collect all LOG()
// events at all levels (including all VLOG levels) from Chrome, Chrome Frame,
// and the test executable itself for each test into a temporary log file.  At
// the conclusion of each test, the contents of the log file are regurgitated to
// stderr iff the test failed.  In any case, the log file is promptly deleted.
//
// Test executables that wish to benefit from the collector's features (to
// produce verbose logs on test failure to aid in diagnosing flaky and/or
// failing tests, for example) must install the collector via
// |InstallTestLogCollector| before running tests (via RUN_ALL_TESTS(),
// TestSuite::Run(), etc).

#ifndef CHROME_TEST_LOGGING_WIN_TEST_LOG_COLLECTOR_H_
#define CHROME_TEST_LOGGING_WIN_TEST_LOG_COLLECTOR_H_

namespace testing {
class UnitTest;
}

namespace logging_win {

// Installs the test log collector into |unit_test| for its lifetime.
// (Use testing::UnitTest::GetInstance() to get the process-wide unit test
// instance.)
void InstallTestLogCollector(testing::UnitTest* unit_test);

}  // namespace logging_win

#endif  // CHROME_TEST_LOGGING_WIN_TEST_LOG_COLLECTOR_H_
