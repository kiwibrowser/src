// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/login/enrollment/enrollment_screen.h"
#include "chrome/browser/chromeos/login/enrollment/mock_enrollment_screen.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/chromeos/login/screens/mock_base_screen_delegate.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/test/js_checker.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/test/wizard_in_process_browser_test.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/ui/webui_login_view.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/chromeos_test_utils.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::_;

namespace chromeos {

class EnrollmentScreenTest : public WizardInProcessBrowserTest {
 public:
  EnrollmentScreenTest()
      : WizardInProcessBrowserTest(OobeScreen::SCREEN_OOBE_ENROLLMENT) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(EnrollmentScreenTest);
};

IN_PROC_BROWSER_TEST_F(EnrollmentScreenTest, TestCancel) {
  ASSERT_TRUE(WizardController::default_controller());

  EnrollmentScreen* enrollment_screen = EnrollmentScreen::Get(
      WizardController::default_controller()->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  base::RunLoop run_loop;
  MockBaseScreenDelegate mock_base_screen_delegate;
  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      &mock_base_screen_delegate;

  ASSERT_EQ(WizardController::default_controller()->current_screen(),
            enrollment_screen);

  EXPECT_CALL(mock_base_screen_delegate,
              OnExit(_, ScreenExitCode::ENTERPRISE_ENROLLMENT_COMPLETED, _))
      .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  enrollment_screen->OnCancel();
  content::RunThisRunLoop(&run_loop);
  Mock::VerifyAndClearExpectations(&mock_base_screen_delegate);

  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      WizardController::default_controller();
}

// Flaky test: crbug.com/394069
IN_PROC_BROWSER_TEST_F(EnrollmentScreenTest, DISABLED_TestSuccess) {
  ASSERT_TRUE(WizardController::default_controller());
  EXPECT_FALSE(StartupUtils::IsOobeCompleted());

  EnrollmentScreen* enrollment_screen = EnrollmentScreen::Get(
      WizardController::default_controller()->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  base::RunLoop run_loop;
  MockBaseScreenDelegate mock_base_screen_delegate;
  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      &mock_base_screen_delegate;

  ASSERT_EQ(WizardController::default_controller()->current_screen(),
            enrollment_screen);

  enrollment_screen->OnDeviceEnrolled("");
  run_loop.RunUntilIdle();
  EXPECT_TRUE(StartupUtils::IsOobeCompleted());

  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      WizardController::default_controller();
}

class AttestationAuthEnrollmentScreenTest : public EnrollmentScreenTest {
 public:
  AttestationAuthEnrollmentScreenTest() {}

 private:
  // Overridden from InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    EnrollmentScreenTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnterpriseEnableZeroTouchEnrollment);
  }

  DISALLOW_COPY_AND_ASSIGN(AttestationAuthEnrollmentScreenTest);
};

IN_PROC_BROWSER_TEST_F(AttestationAuthEnrollmentScreenTest, TestCancel) {
  ASSERT_TRUE(WizardController::default_controller());

  EnrollmentScreen* enrollment_screen = EnrollmentScreen::Get(
      WizardController::default_controller()->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  base::RunLoop run_loop;
  MockBaseScreenDelegate mock_base_screen_delegate;
  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      &mock_base_screen_delegate;

  ASSERT_EQ(WizardController::default_controller()->current_screen(),
            enrollment_screen);

  EXPECT_CALL(mock_base_screen_delegate,
              OnExit(_, ScreenExitCode::ENTERPRISE_ENROLLMENT_COMPLETED, _))
      .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  ASSERT_FALSE(enrollment_screen->AdvanceToNextAuth());
  enrollment_screen->OnCancel();
  content::RunThisRunLoop(&run_loop);
  Mock::VerifyAndClearExpectations(&mock_base_screen_delegate);

  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      WizardController::default_controller();
}

IN_PROC_BROWSER_TEST_F(EnrollmentScreenTest, EnrollmentSpinner) {
  WizardController* wcontroller = WizardController::default_controller();
  ASSERT_TRUE(wcontroller);

  EnrollmentScreen* enrollment_screen =
      EnrollmentScreen::Get(wcontroller->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  EnrollmentScreenView* view = enrollment_screen->GetView();
  ASSERT_TRUE(view);

  test::JSChecker checker(
      LoginDisplayHost::default_host()->GetWebUILoginView()->GetWebContents());

  // Run through the flow
  view->Show();
  OobeScreenWaiter(OobeScreen::SCREEN_OOBE_ENROLLMENT).Wait();
  checker.ExpectTrue(
      "window.getComputedStyle(document.getElementById('oauth-enroll-step-"
      "signin')).display !== 'none'");

  view->ShowEnrollmentSpinnerScreen();
  checker.ExpectTrue(
      "window.getComputedStyle(document.getElementById('oauth-enroll-step-"
      "working')).display !== 'none'");

  view->ShowAttestationBasedEnrollmentSuccessScreen("fake domain");
  checker.ExpectTrue(
      "window.getComputedStyle(document.getElementById('oauth-enroll-step-abe-"
      "success')).display !== 'none'");
}

class ForcedAttestationAuthEnrollmentScreenTest : public EnrollmentScreenTest {
 public:
  ForcedAttestationAuthEnrollmentScreenTest() {}

 private:
  // Overridden from InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    EnrollmentScreenTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        switches::kEnterpriseEnableZeroTouchEnrollment, "forced");
  }

  DISALLOW_COPY_AND_ASSIGN(ForcedAttestationAuthEnrollmentScreenTest);
};

IN_PROC_BROWSER_TEST_F(ForcedAttestationAuthEnrollmentScreenTest, TestCancel) {
  ASSERT_TRUE(WizardController::default_controller());

  EnrollmentScreen* enrollment_screen = EnrollmentScreen::Get(
      WizardController::default_controller()->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  base::RunLoop run_loop;
  MockBaseScreenDelegate mock_base_screen_delegate;
  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      &mock_base_screen_delegate;

  ASSERT_EQ(WizardController::default_controller()->current_screen(),
            enrollment_screen);

  EXPECT_CALL(mock_base_screen_delegate,
              OnExit(_, ScreenExitCode::ENTERPRISE_ENROLLMENT_BACK, _))
      .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  ASSERT_FALSE(enrollment_screen->AdvanceToNextAuth());
  enrollment_screen->OnCancel();
  content::RunThisRunLoop(&run_loop);
  Mock::VerifyAndClearExpectations(&mock_base_screen_delegate);

  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      WizardController::default_controller();
}

class MultiAuthEnrollmentScreenTest : public EnrollmentScreenTest {
 public:
  MultiAuthEnrollmentScreenTest() {}

 private:
  // Overridden from InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    EnrollmentScreenTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnterpriseEnableZeroTouchEnrollment);
    // Kiosk mode will force OAuth enrollment.
    base::FilePath test_data_dir;
    ASSERT_TRUE(chromeos::test_utils::GetTestDataPath(
        "app_mode", "kiosk_manifest", &test_data_dir));
    command_line->AppendSwitchPath(
        switches::kAppOemManifestFile,
        test_data_dir.AppendASCII("kiosk_manifest.json"));
  }

  DISALLOW_COPY_AND_ASSIGN(MultiAuthEnrollmentScreenTest);
};

IN_PROC_BROWSER_TEST_F(MultiAuthEnrollmentScreenTest, TestCancel) {
  ASSERT_TRUE(WizardController::default_controller());

  EnrollmentScreen* enrollment_screen = EnrollmentScreen::Get(
      WizardController::default_controller()->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  base::RunLoop run_loop;
  MockBaseScreenDelegate mock_base_screen_delegate;
  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      &mock_base_screen_delegate;

  ASSERT_EQ(WizardController::default_controller()->current_screen(),
            enrollment_screen);

  EXPECT_CALL(mock_base_screen_delegate,
              OnExit(_, ScreenExitCode::ENTERPRISE_ENROLLMENT_BACK, _))
      .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  ASSERT_TRUE(enrollment_screen->AdvanceToNextAuth());
  enrollment_screen->OnCancel();
  content::RunThisRunLoop(&run_loop);
  Mock::VerifyAndClearExpectations(&mock_base_screen_delegate);

  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      WizardController::default_controller();
}

class ProvisionedEnrollmentScreenTest : public EnrollmentScreenTest {
 public:
  ProvisionedEnrollmentScreenTest() {}

 private:
  // Overridden from InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    EnrollmentScreenTest::SetUpCommandLine(command_line);
    base::FilePath test_data_dir;
    ASSERT_TRUE(chromeos::test_utils::GetTestDataPath(
        "app_mode", "kiosk_manifest", &test_data_dir));
    command_line->AppendSwitchPath(
        switches::kAppOemManifestFile,
        test_data_dir.AppendASCII("kiosk_manifest.json"));
  }

  DISALLOW_COPY_AND_ASSIGN(ProvisionedEnrollmentScreenTest);
};

IN_PROC_BROWSER_TEST_F(ProvisionedEnrollmentScreenTest, TestBackButton) {
  ASSERT_TRUE(WizardController::default_controller());

  EnrollmentScreen* enrollment_screen = EnrollmentScreen::Get(
      WizardController::default_controller()->screen_manager());
  ASSERT_TRUE(enrollment_screen);

  base::RunLoop run_loop;
  MockBaseScreenDelegate mock_base_screen_delegate;
  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      &mock_base_screen_delegate;

  ASSERT_EQ(WizardController::default_controller()->current_screen(),
            enrollment_screen);

  EXPECT_CALL(mock_base_screen_delegate,
              OnExit(_, ScreenExitCode::ENTERPRISE_ENROLLMENT_BACK, _))
      .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  enrollment_screen->OnCancel();
  content::RunThisRunLoop(&run_loop);
  Mock::VerifyAndClearExpectations(&mock_base_screen_delegate);

  static_cast<BaseScreen*>(enrollment_screen)->base_screen_delegate_ =
      WizardController::default_controller();
}

}  // namespace chromeos
