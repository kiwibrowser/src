// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/test/oobe_base_test.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_webui.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "google_apis/gaia/gaia_switches.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/test/ui_controls.h"
#include "ui/views/widget/widget.h"

using namespace net::test_server;

namespace chromeos {

class OobeTest : public OobeBaseTest {
 public:
  OobeTest() {}
  ~OobeTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kOobeSkipPostLogin);

    OobeBaseTest::SetUpCommandLine(command_line);
  }

  void TearDownOnMainThread() override {
    // If the login display is still showing, exit gracefully.
    if (LoginDisplayHost::default_host()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(&chrome::AttemptExit));
      content::RunMessageLoop();
    }

    OobeBaseTest::TearDownOnMainThread();
  }

  LoginDisplayWebUI* GetLoginDisplay() {
    return static_cast<LoginDisplayWebUI*>(
        ExistingUserController::current_controller()->login_display());
  }

  views::Widget* GetLoginWindowWidget() {
    return static_cast<LoginDisplayHostWebUI*>(LoginDisplayHost::default_host())
        ->login_window_for_test();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(OobeTest);
};

IN_PROC_BROWSER_TEST_F(OobeTest, NewUser) {
  WaitForGaiaPageLoad();

  content::WindowedNotificationObserver session_start_waiter(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());

  GetLoginDisplay()->ShowSigninScreenForTest(OobeBaseTest::kFakeUserEmail,
                                             OobeBaseTest::kFakeUserPassword,
                                             OobeBaseTest::kEmptyUserServices);

  session_start_waiter.Wait();
}

IN_PROC_BROWSER_TEST_F(OobeTest, Accelerator) {
  WaitForGaiaPageLoad();

  gfx::NativeWindow login_window = GetLoginWindowWidget()->GetNativeWindow();

  ui_controls::SendKeyPress(login_window, ui::VKEY_E,
                            true,    // control
                            false,   // shift
                            true,    // alt
                            false);  // command
  OobeScreenWaiter(OobeScreen::SCREEN_OOBE_ENROLLMENT).Wait();
}

}  // namespace chromeos
