// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/local_input_monitor.h"

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "remoting/host/client_session_control.h"
#include "remoting/host/host_mock_objects.h"
#include "remoting/protocol/protocol_mock_objects.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

using testing::_;
using testing::AnyNumber;
using testing::ReturnRef;

namespace {

class LocalInputMonitorTest : public testing::Test {
 public:
  LocalInputMonitorTest();

  void SetUp() override;

  base::test::ScopedTaskEnvironment scoped_task_environment_ {
#if defined(OS_WIN)
    base::test::ScopedTaskEnvironment::MainThreadType::UI
#else   // !defined(OS_WIN)
    // Required to watch a file descriptor from NativeMessageProcessHost.
    base::test::ScopedTaskEnvironment::MainThreadType::IO
#endif  // !defined(OS_WIN)
  };

  base::RunLoop run_loop_;
  scoped_refptr<AutoThreadTaskRunner> task_runner_;

  std::string client_jid_;
  MockClientSessionControl client_session_control_;
  base::WeakPtrFactory<ClientSessionControl> client_session_control_factory_;
};

LocalInputMonitorTest::LocalInputMonitorTest()
    : client_jid_("user@domain/rest-of-jid"),
      client_session_control_factory_(&client_session_control_) {}

void LocalInputMonitorTest::SetUp() {
  // Run the task environment until no components depend on it.
  task_runner_ = new AutoThreadTaskRunner(base::ThreadTaskRunnerHandle::Get(),
                                          run_loop_.QuitClosure());
}

}  // namespace

// This test is really to exercise only the creation and destruction code in
// LocalInputMonitor.
TEST_F(LocalInputMonitorTest, Basic) {
  // Ignore all callbacks.
  EXPECT_CALL(client_session_control_, client_jid())
      .Times(AnyNumber())
      .WillRepeatedly(ReturnRef(client_jid_));
  EXPECT_CALL(client_session_control_, DisconnectSession(_))
      .Times(AnyNumber());
  EXPECT_CALL(client_session_control_, OnLocalMouseMoved(_))
      .Times(AnyNumber());
  EXPECT_CALL(client_session_control_, SetDisableInputs(_))
      .Times(0);

  {
    std::unique_ptr<LocalInputMonitor> local_input_monitor =
        LocalInputMonitor::Create(task_runner_, task_runner_, task_runner_,
                                  client_session_control_factory_.GetWeakPtr());
    task_runner_ = nullptr;
  }

  run_loop_.Run();
}

}  // namespace remoting
