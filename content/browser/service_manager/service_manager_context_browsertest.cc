// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/test/launcher/test_launcher.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/test_launcher.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/constants.mojom.h"
#include "services/service_manager/public/mojom/service_manager.mojom.h"
#include "services/test/echo/public/mojom/echo.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::HasSubstr;
using testing::Not;

namespace content {
namespace {

bool ShouldTerminateOnServiceQuit(const service_manager::Identity& id) {
  return id.name() == echo::mojom::kServiceName;
}

class ServiceInstanceListener
    : public service_manager::mojom::ServiceManagerListener {
 public:
  explicit ServiceInstanceListener(
      service_manager::mojom::ServiceManagerListenerRequest request)
      : binding_(this, std::move(request)) {}
  ~ServiceInstanceListener() override = default;

  void WaitForInit() {
    base::RunLoop loop;
    init_wait_loop_ = &loop;
    loop.Run();
    init_wait_loop_ = nullptr;
  }

  uint32_t WaitForServicePID(const std::string& service_name) {
    base::RunLoop loop;
    pid_wait_loop_ = &loop;
    service_expecting_pid_ = service_name;
    loop.Run();
    pid_wait_loop_ = nullptr;
    return pid_received_;
  }

 private:
  // service_manager::mojom::ServiceManagerListener:
  void OnInit(std::vector<service_manager::mojom::RunningServiceInfoPtr>
                  instances) override {
    if (init_wait_loop_)
      init_wait_loop_->Quit();
  }

  void OnServiceCreated(
      service_manager::mojom::RunningServiceInfoPtr instance) override {}
  void OnServiceStarted(const service_manager::Identity&,
                        uint32_t pid) override {}
  void OnServiceFailedToStart(const service_manager::Identity&) override {}
  void OnServiceStopped(const service_manager::Identity&) override {}

  void OnServicePIDReceived(const service_manager::Identity& identity,
                            uint32_t pid) override {
    if (identity.name() == service_expecting_pid_ && pid_wait_loop_) {
      pid_received_ = pid;
      pid_wait_loop_->Quit();
    }
  }

  base::RunLoop* init_wait_loop_ = nullptr;
  base::RunLoop* pid_wait_loop_ = nullptr;
  std::string service_expecting_pid_;
  uint32_t pid_received_ = 0;
  mojo::Binding<service_manager::mojom::ServiceManagerListener> binding_;

  DISALLOW_COPY_AND_ASSIGN(ServiceInstanceListener);
};

}  // namespace

using ServiceManagerContextBrowserTest = ContentBrowserTest;

// "MANUAL" tests only run when kRunManualTestsFlag is set.
IN_PROC_BROWSER_TEST_F(ServiceManagerContextBrowserTest,
                       MANUAL_TerminateOnServiceQuit) {
  ShellContentBrowserClient::Get()
      ->set_should_terminate_on_service_quit_callback(
          base::Bind(&ShouldTerminateOnServiceQuit));

  // Launch a test service.
  echo::mojom::EchoPtr echo_ptr;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(echo::mojom::kServiceName, &echo_ptr);

  base::RunLoop loop;
  // Terminate the service. Browser should exit in response with an error.
  echo_ptr->Quit();
  loop.Run();
}

// Flaky timeout on Linux and Chrome OS ASAN: http://crbug.com/803814,
// crbug.com/804113.
#if (defined(OS_CHROMEOS) || defined(OS_LINUX)) && defined(ADDRESS_SANITIZER)
#define MAYBE_TerminateOnServiceQuit DISABLED_TerminateOnServiceQuit
#elif defined(OS_WIN)
// crbug.com/804937.  Causes failures when test times out even if retry passes.
#define MAYBE_TerminateOnServiceQuit DISABLED_TerminateOnServiceQuit
#else
#define MAYBE_TerminateOnServiceQuit TerminateOnServiceQuit
#endif
TEST(ServiceManagerContextTest, MAYBE_TerminateOnServiceQuit) {
  // Run the above test and collect the test output.
  base::CommandLine new_test =
      base::CommandLine(base::CommandLine::ForCurrentProcess()->GetProgram());
  new_test.AppendSwitchASCII(
      base::kGTestFilterFlag,
      "ServiceManagerContextBrowserTest.MANUAL_TerminateOnServiceQuit");
  new_test.AppendSwitch(kRunManualTestsFlag);
  new_test.AppendSwitch(kSingleProcessTestsFlag);

  base::ScopedAllowBlockingForTesting allow;
  std::string output;
  base::GetAppOutputAndError(new_test, &output);

#if !defined(OS_ANDROID)
  // The test output contains the failure message.
  // TODO(jamescook): The |output| is always empty on Android. I suspect the
  // test runner does logs collection after the program has exited.
  EXPECT_THAT(output, HasSubstr("Terminating because service 'echo' quit"));
#endif
}

IN_PROC_BROWSER_TEST_F(ServiceManagerContextBrowserTest,
                       ServiceProcessReportsPID) {
  service_manager::mojom::ServiceManagerListenerPtr listener_proxy;
  ServiceInstanceListener listener(mojo::MakeRequest(&listener_proxy));

  auto* connector = ServiceManagerConnection::GetForProcess()->GetConnector();

  service_manager::mojom::ServiceManagerPtr service_manager;
  connector->BindInterface(service_manager::mojom::kServiceName,
                           &service_manager);
  service_manager->AddListener(std::move(listener_proxy));
  listener.WaitForInit();

  echo::mojom::EchoPtr echo_ptr;
  connector->BindInterface(echo::mojom::kServiceName, &echo_ptr);

  // PID should be non-zero, confirming that it was indeed properly reported to
  // the Service Manager. If not reported at all, this will hang.
  EXPECT_GT(listener.WaitForServicePID(echo::mojom::kServiceName), 0u);
}

}  // namespace content
