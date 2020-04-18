// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEMO_SETUP_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEMO_SETUP_SCREEN_H_

#include <memory>

#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/demo_setup_screen_view.h"
#include "chrome/browser/chromeos/policy/enrollment_status_chromeos.h"

namespace chromeos {

class BaseScreenDelegate;

// Controlls demo mode setup. The screen can be shown during OOBE. It allows
// user to setup retail demo mode on the device.
class DemoSetupScreen
    : public BaseScreen,
      public EnterpriseEnrollmentHelper::EnrollmentStatusConsumer {
 public:
  DemoSetupScreen(BaseScreenDelegate* base_screen_delegate,
                  DemoSetupScreenView* view);
  ~DemoSetupScreen() override;

  // BaseScreen:
  void Show() override;
  void Hide() override;
  void OnUserAction(const std::string& action_id) override;

  // Called when view is being destroyed. If Screen is destroyed earlier
  // then it has to call Bind(nullptr).
  void OnViewDestroyed(DemoSetupScreenView* view);

  // EnterpriseEnrollmentHelper::EnterpriseStatusConsumer:
  void OnAuthError(const GoogleServiceAuthError& error) override;
  void OnMultipleLicensesAvailable(
      const EnrollmentLicenseMap& licenses) override;
  void OnEnrollmentError(policy::EnrollmentStatus status) override;
  void OnOtherError(EnterpriseEnrollmentHelper::OtherError error) override;
  void OnDeviceEnrolled(const std::string& additional_token) override;
  void OnDeviceAttributeUpdatePermission(bool granted) override;
  void OnDeviceAttributeUploadCompleted(bool success) override;

 private:
  DemoSetupScreenView* view_;

  std::unique_ptr<EnterpriseEnrollmentHelper> enrollment_helper_;

  DISALLOW_COPY_AND_ASSIGN(DemoSetupScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEMO_SETUP_SCREEN_H_
