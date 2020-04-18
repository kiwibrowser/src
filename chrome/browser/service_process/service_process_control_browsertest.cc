// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/service_process/service_process_control.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/process/kill.h"
#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/cloud_print.mojom.h"
#include "chrome/common/service_process_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/version_info/version_info.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"

class ServiceProcessControlBrowserTest
    : public InProcessBrowserTest {
 public:
  ServiceProcessControlBrowserTest() {
  }
  ~ServiceProcessControlBrowserTest() override {}

  void HistogramsCallback() {
    MockHistogramsCallback();
    QuitMessageLoop();
  }

  MOCK_METHOD0(MockHistogramsCallback, void());

 protected:
  void LaunchServiceProcessControl(bool wait) {
    // Launch the process asynchronously.
    ServiceProcessControl::GetInstance()->Launch(
        base::Bind(&ServiceProcessControlBrowserTest::ProcessControlLaunched,
                   base::Unretained(this)),
        base::Bind(
            &ServiceProcessControlBrowserTest::ProcessControlLaunchFailed,
            base::Unretained(this)));

    // Then run the message loop to keep things running.
    if (wait)
      content::RunMessageLoop();
  }

  static void QuitMessageLoop() {
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

  static void CloudPrintInfoCallback(bool enabled,
                                     const std::string& email,
                                     const std::string& proxy_id) {
    QuitMessageLoop();
  }

  void Disconnect() {
    // This will close the IPC connection.
    ServiceProcessControl::GetInstance()->Disconnect();
  }

  void SetUp() override {
    InProcessBrowserTest::SetUp();

    // This should not be needed because TearDown() ends with a closed
    // service_process_, but HistogramsTimeout and Histograms fail without this
    // on Mac.
    service_process_.Close();
  }

  void TearDown() override {
    if (ServiceProcessControl::GetInstance()->IsConnected())
      EXPECT_TRUE(ServiceProcessControl::GetInstance()->Shutdown());
#if defined(OS_MACOSX)
    // ForceServiceProcessShutdown removes the process from launched on Mac.
    ForceServiceProcessShutdown("", 0);
#endif  // OS_MACOSX
    if (service_process_.IsValid()) {
      int exit_code;
      EXPECT_TRUE(service_process_.WaitForExitWithTimeout(
          TestTimeouts::action_max_timeout(), &exit_code));
      EXPECT_EQ(0, exit_code);
      service_process_.Close();
    }

    InProcessBrowserTest::TearDown();
  }

  void ProcessControlLaunched() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::ProcessId service_pid;
    EXPECT_TRUE(GetServiceProcessData(NULL, &service_pid));
    EXPECT_NE(static_cast<base::ProcessId>(0), service_pid);
#if defined(OS_WIN)
    service_process_ =
        base::Process::OpenWithAccess(service_pid,
                                      SYNCHRONIZE | PROCESS_QUERY_INFORMATION);
#else
    service_process_ = base::Process::Open(service_pid);
#endif
    EXPECT_TRUE(service_process_.IsValid());
    // Quit the current message. Post a QuitTask instead of just calling Quit()
    // because this can get invoked in the context of a Launch() call and we
    // may not be in Run() yet.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  }

  void ProcessControlLaunchFailed() {
    ADD_FAILURE();
    // Quit the current message.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  }

 private:
  base::Process service_process_;
};

class RealServiceProcessControlBrowserTest
      : public ServiceProcessControlBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ServiceProcessControlBrowserTest::SetUpCommandLine(command_line);
    base::FilePath exe;
    base::PathService::Get(base::DIR_EXE, &exe);
#if defined(OS_MACOSX)
    exe = exe.DirName().DirName().DirName();
#endif
    exe = exe.Append(chrome::kHelperProcessExecutablePath);
    // Run chrome instead of browser_tests.exe.
    EXPECT_TRUE(base::PathExists(exe));
    command_line->AppendSwitchPath(switches::kBrowserSubprocessPath, exe);
  }
};

// TODO(vitalybuka): Fix crbug.com/340563
IN_PROC_BROWSER_TEST_F(RealServiceProcessControlBrowserTest,
                       DISABLED_LaunchAndIPC) {
  LaunchServiceProcessControl(true);

  // Make sure we are connected to the service process.
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  cloud_print::mojom::CloudPrintPtr cloud_print_proxy;
  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->GetCloudPrintProxyInfo(
      base::Bind(&ServiceProcessControlBrowserTest::CloudPrintInfoCallback));
  content::RunMessageLoop();

  // And then shutdown the service process.
  EXPECT_TRUE(ServiceProcessControl::GetInstance()->Shutdown());
}

IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, LaunchAndIPC) {
  LaunchServiceProcessControl(true);

  // Make sure we are connected to the service process.
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  cloud_print::mojom::CloudPrintPtr cloud_print_proxy;
  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->GetCloudPrintProxyInfo(
      base::Bind(&ServiceProcessControlBrowserTest::CloudPrintInfoCallback));
  content::RunMessageLoop();

  // And then shutdown the service process.
  EXPECT_TRUE(ServiceProcessControl::GetInstance()->Shutdown());
}

IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, LaunchAndReconnect) {
  LaunchServiceProcessControl(true);

  // Make sure we are connected to the service process.
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  // Send an IPC that will keep the service process alive after we disconnect.
  cloud_print::mojom::CloudPrintPtr cloud_print_proxy;
  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->EnableCloudPrintProxyWithRobot(
      "", "", "", base::Value(base::Value::Type::DICTIONARY));

  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->GetCloudPrintProxyInfo(
      base::Bind(&ServiceProcessControlBrowserTest::CloudPrintInfoCallback));
  content::RunMessageLoop();
  Disconnect();

  LaunchServiceProcessControl(false);

  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  content::RunMessageLoop();

  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->GetCloudPrintProxyInfo(
      base::Bind(&ServiceProcessControlBrowserTest::CloudPrintInfoCallback));
  content::RunMessageLoop();

  // And then shutdown the service process.
  EXPECT_TRUE(ServiceProcessControl::GetInstance()->Shutdown());
}

// This tests the case when a service process is launched when the browser
// starts but we try to launch it again while setting up Cloud Print.
// Flaky on Mac. http://crbug.com/517420
#if defined(OS_MACOSX)
#define MAYBE_LaunchTwice DISABLED_LaunchTwice
#else
#define MAYBE_LaunchTwice LaunchTwice
#endif
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, MAYBE_LaunchTwice) {
  // Launch the service process the first time.
  LaunchServiceProcessControl(true);

  // Make sure we are connected to the service process.
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  cloud_print::mojom::CloudPrintPtr cloud_print_proxy;
  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->GetCloudPrintProxyInfo(
      base::Bind(&ServiceProcessControlBrowserTest::CloudPrintInfoCallback));
  content::RunMessageLoop();

  // Launch the service process again.
  LaunchServiceProcessControl(true);
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  ServiceProcessControl::GetInstance()->remote_interfaces().GetInterface(
      &cloud_print_proxy);
  cloud_print_proxy->GetCloudPrintProxyInfo(
      base::Bind(&ServiceProcessControlBrowserTest::CloudPrintInfoCallback));
  content::RunMessageLoop();
}

static void DecrementUntilZero(int* count) {
  (*count)--;
  if (!(*count))
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
}

// Flaky on Mac. http://crbug.com/517420
#if defined(OS_MACOSX)
#define MAYBE_MultipleLaunchTasks DISABLED_MultipleLaunchTasks
#else
#define MAYBE_MultipleLaunchTasks MultipleLaunchTasks
#endif
// Invoke multiple Launch calls in succession and ensure that all the tasks
// get invoked.
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest,
                       MAYBE_MultipleLaunchTasks) {
  ServiceProcessControl* process = ServiceProcessControl::GetInstance();
  int launch_count = 5;
  for (int i = 0; i < launch_count; i++) {
    // Launch the process asynchronously.
    process->Launch(base::Bind(&DecrementUntilZero, &launch_count),
                    base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  }
  // Then run the message loop to keep things running.
  content::RunMessageLoop();
  EXPECT_EQ(0, launch_count);
}

// Flaky on Mac. http://crbug.com/517420
#if defined(OS_MACOSX)
#define MAYBE_SameLaunchTask DISABLED_SameLaunchTask
#else
#define MAYBE_SameLaunchTask SameLaunchTask
#endif
// Make sure using the same task for success and failure tasks works.
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, MAYBE_SameLaunchTask) {
  ServiceProcessControl* process = ServiceProcessControl::GetInstance();
  int launch_count = 5;
  for (int i = 0; i < launch_count; i++) {
    // Launch the process asynchronously.
    base::Closure task = base::Bind(&DecrementUntilZero, &launch_count);
    process->Launch(task, task);
  }
  // Then run the message loop to keep things running.
  content::RunMessageLoop();
  EXPECT_EQ(0, launch_count);
}

// Tests whether disconnecting from the service IPC causes the service process
// to die.
// Flaky on Mac. http://crbug.com/517420
#if defined(OS_MACOSX)
#define MAYBE_DieOnDisconnect DISABLED_DieOnDisconnect
#else
#define MAYBE_DieOnDisconnect DieOnDisconnect
#endif
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest,
                       MAYBE_DieOnDisconnect) {
  // Launch the service process.
  LaunchServiceProcessControl(true);
  // Make sure we are connected to the service process.
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  Disconnect();
}

// Flaky on Mac. http://crbug.com/517420
#if defined(OS_MACOSX)
#define MAYBE_ForceShutdown DISABLED_ForceShutdown
#else
#define MAYBE_ForceShutdown ForceShutdown
#endif
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, MAYBE_ForceShutdown) {
  // Launch the service process.
  LaunchServiceProcessControl(true);
  // Make sure we are connected to the service process.
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  base::ProcessId service_pid;
  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(GetServiceProcessData(NULL, &service_pid));
  EXPECT_NE(static_cast<base::ProcessId>(0), service_pid);
  ForceServiceProcessShutdown(version_info::GetVersionNumber(), service_pid);
}

// Flaky on Mac. http://crbug.com/517420
#if defined(OS_MACOSX)
#define MAYBE_CheckPid DISABLED_CheckPid
#else
#define MAYBE_CheckPid CheckPid
#endif
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, MAYBE_CheckPid) {
  base::ProcessId service_pid;
  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_FALSE(GetServiceProcessData(NULL, &service_pid));
  // Launch the service process.
  LaunchServiceProcessControl(true);
  EXPECT_TRUE(GetServiceProcessData(NULL, &service_pid));
  EXPECT_NE(static_cast<base::ProcessId>(0), service_pid);
  // Disconnect from service process.
  Disconnect();
}

IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, HistogramsNoService) {
  ASSERT_FALSE(ServiceProcessControl::GetInstance()->IsConnected());
  EXPECT_CALL(*this, MockHistogramsCallback()).Times(0);
  EXPECT_FALSE(ServiceProcessControl::GetInstance()->GetHistograms(
      base::Bind(&ServiceProcessControlBrowserTest::HistogramsCallback,
                 base::Unretained(this)),
      base::TimeDelta()));
}

// Histograms disabled on OSX http://crbug.com/406227
#if defined(OS_MACOSX)
#define MAYBE_HistogramsTimeout DISABLED_HistogramsTimeout
#define MAYBE_Histograms DISABLED_Histograms
#else
#define MAYBE_HistogramsTimeout HistogramsTimeout
#define MAYBE_Histograms Histograms
#endif
IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest,
                       MAYBE_HistogramsTimeout) {
  LaunchServiceProcessControl(true);
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  // Callback should not be called during GetHistograms call.
  EXPECT_CALL(*this, MockHistogramsCallback()).Times(0);
  EXPECT_TRUE(ServiceProcessControl::GetInstance()->GetHistograms(
      base::Bind(&ServiceProcessControlBrowserTest::HistogramsCallback,
                 base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(100)));
  EXPECT_CALL(*this, MockHistogramsCallback()).Times(1);
  EXPECT_TRUE(ServiceProcessControl::GetInstance()->Shutdown());
  content::RunMessageLoop();
}

IN_PROC_BROWSER_TEST_F(ServiceProcessControlBrowserTest, MAYBE_Histograms) {
  LaunchServiceProcessControl(true);
  ASSERT_TRUE(ServiceProcessControl::GetInstance()->IsConnected());
  // Callback should not be called during GetHistograms call.
  EXPECT_CALL(*this, MockHistogramsCallback()).Times(0);
  // Wait for real callback by providing large timeout value.
  EXPECT_TRUE(ServiceProcessControl::GetInstance()->GetHistograms(
      base::Bind(&ServiceProcessControlBrowserTest::HistogramsCallback,
                base::Unretained(this)),
      base::TimeDelta::FromHours(1)));
  EXPECT_CALL(*this, MockHistogramsCallback()).Times(1);
  content::RunMessageLoop();
}
