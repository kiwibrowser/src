// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_LAUNCHER_H_
#define CONTENT_PUBLIC_TEST_TEST_LAUNCHER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/test/launcher/test_launcher.h"

namespace base {
class CommandLine;
class FilePath;
}

namespace content {
class ContentMainDelegate;
struct ContentMainParams;

extern const char kEmptyTestName[];
extern const char kHelpFlag[];
extern const char kLaunchAsBrowser[];
extern const char kRunManualTestsFlag[];
extern const char kSingleProcessTestsFlag[];

// Flag that causes only the kEmptyTestName test to be run.
extern const char kWarmupFlag[];

// See details in PreRunTest().
class TestState {
 public:
  virtual ~TestState() {}

  // Called once test process has launched (and is still running).
  // NOTE: this is called on a background thread.
  virtual void ChildProcessLaunched(base::ProcessHandle handle,
                                    base::ProcessId pid) = 0;
};

class TestLauncherDelegate {
 public:
  virtual int RunTestSuite(int argc, char** argv) = 0;
  virtual bool AdjustChildProcessCommandLine(
      base::CommandLine* command_line,
      const base::FilePath& temp_data_dir) = 0;
  virtual ContentMainDelegate* CreateContentMainDelegate() = 0;

  // Called prior to running each test. The delegate may alter the CommandLine
  // and options used to launch the subprocess. Additionally the client may
  // return a TestState that is destroyed once the test completes as well as
  // once the test process is launched.
  //
  // NOTE: this is not called if --single_process is supplied.
  virtual std::unique_ptr<TestState> PreRunTest(
      base::CommandLine* command_line,
      base::TestLauncher::LaunchOptions* test_launch_options);

  // Called after running each test. Can modify test result.
  //
  // NOTE: Just like PreRunTest, this is not called when --single_process is
  // supplied.
  virtual void PostRunTest(base::TestResult* result) {}

  // Allows a TestLauncherDelegate to do work before the launcher shards test
  // jobs.
  virtual void PreSharding() {}

  // Invoked when a child process times out immediately before it is terminated.
  // |command_line| is the command line of the child process.
  virtual void OnTestTimedOut(const base::CommandLine& command_line) {}

  // Called prior to returning from LaunchTests(). Gives the delegate a chance
  // to do cleanup before state created by TestLauncher has been destroyed (such
  // as the AtExitManager).
  virtual void OnDoneRunningTests() {}

 protected:
  virtual ~TestLauncherDelegate() = default;
};

// Launches tests using |launcher_delegate|. |parallel_jobs| is the number
// of test jobs to be run in parallel.
int LaunchTests(TestLauncherDelegate* launcher_delegate,
                size_t parallel_jobs,
                int argc,
                char** argv) WARN_UNUSED_RESULT;

TestLauncherDelegate* GetCurrentTestLauncherDelegate();
ContentMainParams* GetContentMainParams();

// Returns true if the currently running test has a prefix that indicates it
// should run before a test of the same name without the prefix.
bool IsPreTest();

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_LAUNCHER_H_
