// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_watcher_command_line_win.h"

#include <windows.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/win/scoped_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

base::FilePath ExampleExe() {
  static const wchar_t kExampleExe[] = FILE_PATH_LITERAL("example.exe");
  base::FilePath example_exe(kExampleExe);
  return example_exe;
}

}  // namespace

class TestChromeWatcherCommandLineGenerator
    : public ChromeWatcherCommandLineGenerator {
 public:
  TestChromeWatcherCommandLineGenerator()
      : ChromeWatcherCommandLineGenerator(ExampleExe()) {
  }

  // In the normal case the generator and interpreter are run in separate
  // processes. However, since they are both being run in the same process in
  // these tests the handle tracker will explode as they both claim ownership of
  // the same handle. This function allows the generator handles to be released
  // before they are subsequently claimed by an interpreter.
  void ReleaseHandlesWithoutClosing() {
    on_initialized_event_handle_.Take();
    parent_process_handle_.Take();
  }
};

// A fixture for tests that involve parsed command-lines. Contains utility
// functions for standing up a well-configured valid generator.
class ChromeWatcherCommandLineTest : public testing::Test {
 public:
  ChromeWatcherCommandLineTest()
      : cmd_line_(base::CommandLine::NO_PROGRAM) {
  }

  void GenerateAndInterpretCommandLine() {
    process_ = base::Process::OpenWithAccess(
      base::GetCurrentProcId(), PROCESS_QUERY_INFORMATION | SYNCHRONIZE);
    ASSERT_TRUE(process_.Handle());

    event_.Set(::CreateEvent(nullptr, FALSE, FALSE, nullptr));
    ASSERT_TRUE(event_.IsValid());

    // The above handles are duplicated by the generator.
    generator_.SetOnInitializedEventHandle(event_.Get());
    generator_.SetParentProcessHandle(process_.Handle());

    // Expect there to be two inherited handles created by the generator.
    std::vector<HANDLE> handles;
    generator_.GetInheritedHandles(&handles);
    EXPECT_EQ(2U, handles.size());

    cmd_line_ = generator_.GenerateCommandLine();

    // In the normal case the generator and interpreter are run in separate
    // processes. However, since they are both being run in the same process in
    // this test that will lead to both the generator and the interpreter
    // claiming ownership of the same handles, as far as the handle tracker is
    // concerned. To prevent this the handles are first released from tracking
    // by the generator.
    generator_.ReleaseHandlesWithoutClosing();

    interpreted_ = ChromeWatcherCommandLine::InterpretCommandLine(cmd_line_);
    EXPECT_TRUE(interpreted_);
  }

  base::Process process_;
  base::win::ScopedHandle event_;
  TestChromeWatcherCommandLineGenerator generator_;
  base::CommandLine cmd_line_;
  std::unique_ptr<ChromeWatcherCommandLine> interpreted_;
};

// The corresponding death test fixture.
using ChromeWatcherCommandLineDeathTest = ChromeWatcherCommandLineTest;

TEST(ChromeWatcherCommandLineGeneratorTest, UseOfBadHandlesFails) {
  // Handles are always machine word aligned so there is no way these are valid.
  HANDLE bad_handle_1 = reinterpret_cast<HANDLE>(0x01FC00B1);
  HANDLE bad_handle_2 = reinterpret_cast<HANDLE>(0x01FC00B3);

  // The above handles are duplicated by the generator.
  TestChromeWatcherCommandLineGenerator generator;
  EXPECT_FALSE(generator.SetOnInitializedEventHandle(bad_handle_1));
  EXPECT_FALSE(generator.SetParentProcessHandle(bad_handle_2));

  // Expect there to be no inherited handles created by the generator.
  std::vector<HANDLE> handles;
  generator.GetInheritedHandles(&handles);
  EXPECT_TRUE(handles.empty());
}

TEST(ChromeWatcherCommandLineGeneratorTest, BadCommandLineFailsInterpretation) {
  // Create an invalid command-line that is missing several fields.
  base::CommandLine cmd_line(ExampleExe());

  // Parse the command line.
  auto interpreted = ChromeWatcherCommandLine::InterpretCommandLine(cmd_line);
  EXPECT_FALSE(interpreted);
}

TEST_F(ChromeWatcherCommandLineDeathTest, HandlesLeftUntakenCausesDeath) {
  EXPECT_NO_FATAL_FAILURE(GenerateAndInterpretCommandLine());

  // Leave the handles in the interpreter and expect it to explode upon
  // destruction.
  // String arguments aren't passed to CHECK() in official builds.
#if defined(OFFICIAL_BUILD) && defined(NDEBUG)
  EXPECT_DEATH(interpreted_.reset(), "");
#else
  EXPECT_DEATH(interpreted_.reset(), "Handles left untaken.");
#endif

  // The above call to the destructor only runs in the context of the death test
  // child process. To prevent the parent process from exploding in a similar
  // fashion, release the handles so the destructor is happy.
  interpreted_->TakeOnInitializedEventHandle().Close();
  interpreted_->TakeParentProcessHandle().Close();
}

TEST_F(ChromeWatcherCommandLineTest, SuccessfulParse) {
  EXPECT_NO_FATAL_FAILURE(GenerateAndInterpretCommandLine());

  EXPECT_EQ(::GetCurrentThreadId(), interpreted_->main_thread_id());

  // Explicitly take the handles from the interpreter so it doesn't explode.
  base::win::ScopedHandle on_init =
      interpreted_->TakeOnInitializedEventHandle();
  base::win::ScopedHandle proc = interpreted_->TakeParentProcessHandle();
  EXPECT_TRUE(on_init.IsValid());
  EXPECT_TRUE(proc.IsValid());
}

// TODO(chrisha): Remove this test upon switching to using the new command-line
// classes.
TEST(OldChromeWatcherCommandLineTest, BasicTest) {
  // Ownership of these handles is passed to the ScopedHandles below via
  // InterpretChromeWatcherCommandLine().
  base::ProcessHandle current =
      ::OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                    TRUE,  // Inheritable.
                    ::GetCurrentProcessId());
  ASSERT_NE(nullptr, current);

  HANDLE event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
  ASSERT_NE(nullptr, event);
  DWORD current_thread_id = ::GetCurrentThreadId();
  base::CommandLine cmd_line = GenerateChromeWatcherCommandLine(
      ExampleExe(), current, current_thread_id, event);

  base::win::ScopedHandle current_result;
  DWORD current_thread_id_result = 0;
  base::win::ScopedHandle event_result;
  ASSERT_TRUE(InterpretChromeWatcherCommandLine(
      cmd_line, &current_result, &current_thread_id_result, &event_result));
  ASSERT_EQ(current, current_result.Get());
  ASSERT_EQ(current_thread_id, current_thread_id_result);
  ASSERT_EQ(event, event_result.Get());
}
