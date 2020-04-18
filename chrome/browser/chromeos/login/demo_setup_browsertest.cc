// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/screens/demo_setup_screen.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/test/browser_test_utils.h"

namespace chromeos {

// Basic tests for demo mode setup flow.
class DemoSetupTest : public LoginManagerTest {
 public:
  DemoSetupTest() : LoginManagerTest(false) {}
  ~DemoSetupTest() override = default;

  // LoginTestManager:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    LoginManagerTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(chromeos::switches::kEnableDemoMode);
  }

  void ShowDemoSetupScreen() {
    ASSERT_TRUE(JSExecute("cr.ui.Oobe.handleAccelerator('demo_mode');"));
    OobeScreenWaiter(OobeScreen::SCREEN_OOBE_DEMO_SETUP).Wait();
    ASSERT_NE(nullptr, GetDemoSetupScreen());
  }

  DemoSetupScreen* GetDemoSetupScreen() {
    return static_cast<DemoSetupScreen*>(
        WizardController::default_controller()->screen_manager()->GetScreen(
            OobeScreen::SCREEN_OOBE_ENROLLMENT));
  }

  bool IsDemoSetupShown() {
    return js_checker().GetBool(
        "!document.querySelector('#demo-setup').hidden");
  }

 private:
  bool JSExecute(const std::string& script) {
    return content::ExecuteScript(web_contents(), script);
  }

  DISALLOW_COPY_AND_ASSIGN(DemoSetupTest);
};

IN_PROC_BROWSER_TEST_F(DemoSetupTest, ShowDemoSetupScreen) {
  EXPECT_FALSE(IsDemoSetupShown());
  ASSERT_NO_FATAL_FAILURE(ShowDemoSetupScreen());
  EXPECT_TRUE(IsDemoSetupShown());
}

}  // namespace chromeos
