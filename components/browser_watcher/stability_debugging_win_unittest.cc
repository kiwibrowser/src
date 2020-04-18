// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/stability_debugging.h"

#include <windows.h>

#include "base/command_line.h"
#include "base/debug/activity_tracker.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/process/process.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_timeouts.h"
#include "build/build_config.h"
#include "components/browser_watcher/stability_report.pb.h"
#include "components/browser_watcher/stability_report_extractor.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace browser_watcher {

using base::debug::GlobalActivityTracker;

const int kMemorySize = 1 << 20;  // 1MiB
const uint32_t exception_code = 42U;
const uint32_t exception_flag_continuable = 0U;

class StabilityDebuggingTest : public testing::Test {
 public:
  StabilityDebuggingTest() {}
  ~StabilityDebuggingTest() override {
    GlobalActivityTracker* global_tracker = GlobalActivityTracker::Get();
    if (global_tracker) {
      global_tracker->ReleaseTrackerForCurrentThreadForTesting();
      delete global_tracker;
    }
  }

  void SetUp() override {
    testing::Test::SetUp();

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    debug_path_ = temp_dir_.GetPath().AppendASCII("debug.pma");

    GlobalActivityTracker::CreateWithFile(debug_path_, kMemorySize, 0ULL, "",
                                          3);
  }

  const base::FilePath& debug_path() { return debug_path_; }

 private:
  base::ScopedTempDir temp_dir_;
  base::FilePath debug_path_;
};

#if defined(ADDRESS_SANITIZER) && defined(OS_WIN)
// The test does not pass under WinASan. See crbug.com/809524.
#define MAYBE_CrashingTest DISABLED_CrashingTest
#else
#define MAYBE_CrashingTest CrashingTest
#endif
TEST_F(StabilityDebuggingTest, MAYBE_CrashingTest) {
  RegisterStabilityVEH();

  // Raise an exception, then continue.
  __try {
    ::RaiseException(exception_code, exception_flag_continuable, 0U, nullptr);
  } __except (EXCEPTION_CONTINUE_EXECUTION) {
  }

  // Collect the report.
  StabilityReport report;
  ASSERT_EQ(SUCCESS, Extract(debug_path(), &report));

  // Validate expectations.
  ASSERT_EQ(1, report.process_states_size());
  const ProcessState& process_state = report.process_states(0);
  ASSERT_EQ(1, process_state.threads_size());

  bool thread_found = false;
  for (const ThreadState& thread : process_state.threads()) {
    if (thread.thread_id() == ::GetCurrentThreadId()) {
      thread_found = true;
      ASSERT_TRUE(thread.has_exception());
      const Exception& exception = thread.exception();
      EXPECT_EQ(exception_code, exception.code());
      EXPECT_NE(0ULL, exception.program_counter());
      EXPECT_NE(0ULL, exception.exception_address());
      EXPECT_NE(0LL, exception.time());
    }
  }
  ASSERT_TRUE(thread_found);
}

}  // namespace browser_watcher
