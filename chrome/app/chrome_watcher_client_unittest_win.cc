// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_watcher_client_win.h"

#include <windows.h>
#include <string>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/process/process_handle.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/multiprocess_test.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "base/win/scoped_handle.h"
#include "base/win/win_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace {

const char kParentHandle[] = "parent-handle";
const char kEventHandle[] = "event-handle";
const char kNamedEventSuffix[] = "named-event-suffix";

const base::char16 kExitEventBaseName[] = L"ChromeWatcherClientTestExitEvent_";
const base::char16 kInitializeEventBaseName[] =
    L"ChromeWatcherClientTestInitializeEvent_";

base::win::ScopedHandle InterpretHandleSwitch(base::CommandLine& cmd_line,
                                              const char* switch_name) {
  std::string str_handle =
      cmd_line.GetSwitchValueASCII(switch_name);
  if (str_handle.empty()) {
    LOG(ERROR) << "Switch " << switch_name << " unexpectedly absent.";
    return base::win::ScopedHandle();
  }

  unsigned int_handle = 0;
  if (!base::StringToUint(str_handle, &int_handle)) {
    LOG(ERROR) << "Switch " << switch_name << " has invalid value "
               << str_handle;
    return base::win::ScopedHandle();
  }

  return base::win::ScopedHandle(
      reinterpret_cast<base::ProcessHandle>(int_handle));
}

// Simulates a Chrome watcher process. Exits when the global exit event is
// signaled. Signals the "on initialized" event (passed on the command-line)
// when the global initialization event is signaled.
MULTIPROCESS_TEST_MAIN(ChromeWatcherClientTestProcess) {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();

  base::string16 named_event_suffix =
      base::ASCIIToUTF16(cmd_line->GetSwitchValueASCII(kNamedEventSuffix));
  if (named_event_suffix.empty()) {
    LOG(ERROR) << "Switch " << kNamedEventSuffix << " unexpectedly absent.";
    return 1;
  }

  base::win::ScopedHandle exit_event(::CreateEvent(
      NULL, FALSE, FALSE, (kExitEventBaseName + named_event_suffix).c_str()));
  if (!exit_event.IsValid()) {
    LOG(ERROR) << "Failed to create event named "
               << kExitEventBaseName + named_event_suffix;
    return 1;
  }

  base::win::ScopedHandle initialize_event(
      ::CreateEvent(NULL, FALSE, FALSE,
                    (kInitializeEventBaseName + named_event_suffix).c_str()));
  if (!initialize_event.IsValid()) {
    LOG(ERROR) << "Failed to create event named "
               << kInitializeEventBaseName + named_event_suffix;
    return 1;
  }

  base::win::ScopedHandle parent_process(
      InterpretHandleSwitch(*cmd_line, kParentHandle));
  if (!parent_process.IsValid())
    return 1;

  base::win::ScopedHandle on_initialized_event(
      InterpretHandleSwitch(*cmd_line, kEventHandle));
  if (!on_initialized_event.IsValid())
    return 1;

  while (true) {
    // We loop as a convenient way to continue waiting for the exit_event after
    // the initialize_event is signaled. We expect to get initialize_event zero
    // or one times before exit_event, never more.
    HANDLE handles[] = {exit_event.Get(), initialize_event.Get()};
    DWORD result =
        ::WaitForMultipleObjects(arraysize(handles), handles, FALSE, INFINITE);
    switch (result) {
      case WAIT_OBJECT_0:
        // exit_event
        return 0;
      case WAIT_OBJECT_0 + 1:
        // initialize_event
        ::SetEvent(on_initialized_event.Get());
        break;
      case WAIT_FAILED:
        PLOG(ERROR) << "Unexpected failure in WaitForMultipleObjects.";
        return 1;
      default:
        NOTREACHED() << "Unexpected result from WaitForMultipleObjects: "
                     << result;
        return 1;
    }
  }
}

// Implements a thread to launch the ChromeWatcherClient and block on
// EnsureInitialized. Provides various helpers to interact with the
// ChromeWatcherClient.
class ChromeWatcherClientThread : public base::SimpleThread {
 public:
  ChromeWatcherClientThread()
      : SimpleThread("ChromeWatcherClientTest thread"),
        client_(base::Bind(&ChromeWatcherClientThread::GenerateCommandLine,
                           base::Unretained(this))),
        complete_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                  base::WaitableEvent::InitialState::NOT_SIGNALED),
        result_(false) {}

  // Waits up to |timeout| for the call to EnsureInitialized to complete. If it
  // does, sets |result| to the return value of EnsureInitialized and returns
  // true. Otherwise returns false.
  bool WaitForResultWithTimeout(base::TimeDelta timeout, bool* result) {
    if (!complete_.TimedWait(timeout))
      return false;
    *result = result_;
    return true;
  }

  // Waits indefinitely for the call to WaitForInitialization to complete.
  // Returns the return value of WaitForInitialization.
  bool WaitForResult() {
    complete_.Wait();
    return result_;
  }

  ChromeWatcherClient& client() { return client_; }

  base::string16 NamedEventSuffix() {
    return base::UintToString16(base::GetCurrentProcId());
  }

  // base::SimpleThread implementation.
  void Run() override {
    result_ = client_.LaunchWatcher();
    if (result_)
      result_ = client_.EnsureInitialized();
    complete_.Signal();
  }

 private:
  // Returns a command line to launch back into ChromeWatcherClientTestProcess.
  base::CommandLine GenerateCommandLine(HANDLE parent_handle,
                                        DWORD main_thread_id,
                                        HANDLE on_initialized_event) {
    base::CommandLine ret = base::GetMultiProcessTestChildBaseCommandLine();
    ret.AppendSwitchASCII(switches::kTestChildProcess,
                          "ChromeWatcherClientTestProcess");
    ret.AppendSwitchASCII(
        kEventHandle,
        base::UintToString(base::win::HandleToUint32(on_initialized_event)));
    ret.AppendSwitchASCII(
        kParentHandle,
        base::UintToString(base::win::HandleToUint32(parent_handle)));

    // Our child does not actually need the main thread ID, but we verify here
    // that the correct ID is being passed from the client.
    EXPECT_EQ(::GetCurrentThreadId(), main_thread_id);

    ret.AppendSwitchASCII(kNamedEventSuffix,
                          base::UTF16ToASCII(NamedEventSuffix()));
    return ret;
  }

  // The instance under test.
  ChromeWatcherClient client_;
  // Signaled when WaitForInitialization returns.
  base::WaitableEvent complete_;
  // The return value of WaitForInitialization.
  bool result_;

  DISALLOW_COPY_AND_ASSIGN(ChromeWatcherClientThread);
};

}  // namespace

class ChromeWatcherClientTest : public testing::Test {
 protected:
  // Sends a signal to the simulated watcher process to exit. Returns true if
  // successful.
  bool SignalExit() { return ::SetEvent(exit_event_.Get()) != FALSE; }

  // Sends a signal to the simulated watcher process to signal its
  // "initialization". Returns true if successful.
  bool SignalInitialize() {
    return ::SetEvent(initialize_event_.Get()) != FALSE;
  }

  // The helper thread, which also provides access to the ChromeWatcherClient.
  ChromeWatcherClientThread& thread() { return thread_; }

  // testing::Test implementation.
  void SetUp() override {
    exit_event_.Set(::CreateEvent(
        NULL, FALSE, FALSE,
        (kExitEventBaseName + thread_.NamedEventSuffix()).c_str()));
    ASSERT_TRUE(exit_event_.IsValid());
    initialize_event_.Set(::CreateEvent(
        NULL, FALSE, FALSE,
        (kInitializeEventBaseName + thread_.NamedEventSuffix()).c_str()));
    ASSERT_TRUE(initialize_event_.IsValid());
  }

  void TearDown() override {
    // Even if we never launched, the following is harmless.
    SignalExit();
    thread_.client().WaitForExit(nullptr);
    thread_.Join();
  }

 private:
  // Used to launch and block on the Chrome watcher process in a background
  // thread.
  ChromeWatcherClientThread thread_;
  // Used to signal the Chrome watcher process to exit.
  base::win::ScopedHandle exit_event_;
  // Used to signal the Chrome watcher process to signal its own
  // initialization..
  base::win::ScopedHandle initialize_event_;
};

TEST_F(ChromeWatcherClientTest, SuccessTest) {
  thread().Start();
  bool result = false;
  // Give a broken implementation a chance to exit unexpectedly.
  ASSERT_FALSE(thread().WaitForResultWithTimeout(
      base::TimeDelta::FromMilliseconds(100), &result));
  ASSERT_TRUE(SignalInitialize());
  ASSERT_TRUE(thread().WaitForResult());
  // The watcher should still be running. Give a broken implementation a chance
  // to exit unexpectedly, then signal it to exit.
  int exit_code = 0;
  ASSERT_FALSE(thread().client().WaitForExitWithTimeout(
      base::TimeDelta::FromMilliseconds(100), &exit_code));
  SignalExit();
  ASSERT_TRUE(thread().client().WaitForExit(&exit_code));
  ASSERT_EQ(0, exit_code);
}

TEST_F(ChromeWatcherClientTest, FailureTest) {
  thread().Start();
  bool result = false;
  // Give a broken implementation a chance to exit unexpectedly.
  ASSERT_FALSE(thread().WaitForResultWithTimeout(
      base::TimeDelta::FromMilliseconds(100), &result));
  ASSERT_TRUE(SignalExit());
  ASSERT_FALSE(thread().WaitForResult());
  int exit_code = 0;
  ASSERT_TRUE(
      thread().client().WaitForExitWithTimeout(base::TimeDelta(), &exit_code));
  ASSERT_EQ(0, exit_code);
}
