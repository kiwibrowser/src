// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/mach_broker_mac.h"

#include "base/command_line.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_timeouts.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace content {

class MachBrokerTest : public testing::Test,
                       public base::PortProvider::Observer {
 public:
  MachBrokerTest()
      : event_(base::WaitableEvent::ResetPolicy::MANUAL,
               base::WaitableEvent::InitialState::NOT_SIGNALED),
        received_process_(base::kNullProcessHandle) {
    broker_.AddObserver(this);
  }
  ~MachBrokerTest() override {
    broker_.RemoveObserver(this);
  }

  // Helper function to acquire/release locks and call |PlaceholderForPid()|.
  void AddPlaceholderForPid(base::ProcessHandle pid, int child_process_id) {
    base::AutoLock lock(broker_.GetLock());
    broker_.AddPlaceholderForPid(pid, child_process_id);
  }

  void InvalidateChildProcessId(int child_process_id) {
    broker_.InvalidateChildProcessId(child_process_id);
  }

  int GetChildProcessCount(int child_process_id) {
    return broker_.child_process_id_map_.count(child_process_id);
  }

  base::Process LaunchTestChild(const std::string& function,
                                int child_process_id) {
    base::AutoLock lock(broker_.GetLock());
    base::Process test_child_process = base::SpawnMultiProcessTestChild(
        function, base::GetMultiProcessTestChildBaseCommandLine(),
        base::LaunchOptions());
    broker_.AddPlaceholderForPid(test_child_process.Handle(), child_process_id);
    return test_child_process;
  }

  void WaitForChildExit(base::Process& process) {
    int rv = -1;
    ASSERT_TRUE(process.WaitForExitWithTimeout(
                TestTimeouts::action_timeout(), &rv));
    EXPECT_EQ(0, rv);
  }

  void WaitForTaskPort() {
    event_.Wait();
  }

  // base::PortProvider::Observer:
  void OnReceivedTaskPort(base::ProcessHandle process) override {
    received_process_ = process;
    event_.Signal();
  }

 protected:
  MachBroker broker_;
  base::WaitableEvent event_;
  base::ProcessHandle received_process_;
  TestBrowserThreadBundle thread_bundle_;
};

MULTIPROCESS_TEST_MAIN(MachBrokerTestChild) {
  CHECK(MachBroker::ChildSendTaskPortToParent());
  return 0;
}

TEST_F(MachBrokerTest, Locks) {
  // Acquire and release the locks.  Nothing bad should happen.
  base::AutoLock lock(broker_.GetLock());
}

TEST_F(MachBrokerTest, AddChildProcess) {
  {
    base::AutoLock lock(broker_.GetLock());
    broker_.EnsureRunning();
  }
  base::Process child_process = LaunchTestChild("MachBrokerTestChild", 7);
  WaitForTaskPort();
  EXPECT_EQ(child_process.Handle(), received_process_);
  WaitForChildExit(child_process);

  EXPECT_NE(static_cast<mach_port_t>(MACH_PORT_NULL),
            broker_.TaskForPid(child_process.Handle()));
  EXPECT_EQ(1, GetChildProcessCount(7));

  // Should be no entry for any other PID.
  EXPECT_EQ(static_cast<mach_port_t>(MACH_PORT_NULL),
            broker_.TaskForPid(child_process.Handle() + 1));

  InvalidateChildProcessId(7);
  EXPECT_EQ(static_cast<mach_port_t>(MACH_PORT_NULL),
            broker_.TaskForPid(child_process.Handle()));
  EXPECT_EQ(0, GetChildProcessCount(7));
}

}  // namespace content
