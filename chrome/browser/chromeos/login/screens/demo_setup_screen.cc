// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/demo_setup_screen.h"

#include "chrome/browser/chromeos/login/enrollment/enterprise_enrollment_helper.h"
#include "chrome/browser/chromeos/login/screen_manager.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/policy/enrollment_config.h"

namespace {

constexpr const char kUserActionOnlineSetup[] = "online-setup";
constexpr const char kUserActionOfflineSetup[] = "offline-setup";
constexpr const char kUserActionClose[] = "close-setup";
constexpr const char kDemoModeDomain[] = "cros-demo-mode.com";

}  // namespace

namespace chromeos {

DemoSetupScreen::DemoSetupScreen(BaseScreenDelegate* base_screen_delegate,
                                 DemoSetupScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_OOBE_DEMO_SETUP),
      view_(view) {
  DCHECK(view_);
  view_->Bind(this);
}

DemoSetupScreen::~DemoSetupScreen() {
  if (view_)
    view_->Bind(nullptr);
}

void DemoSetupScreen::Show() {
  if (view_)
    view_->Show();
}

void DemoSetupScreen::Hide() {
  if (view_)
    view_->Hide();
}

void DemoSetupScreen::OnAuthError(const GoogleServiceAuthError& error) {
  NOTREACHED();
}

void DemoSetupScreen::OnMultipleLicensesAvailable(
    const EnrollmentLicenseMap& licenses) {
  NOTREACHED();
}

void DemoSetupScreen::OnEnrollmentError(policy::EnrollmentStatus status) {
  LOG(ERROR) << "Enrollment error: " << status.status() << ", "
             << status.client_status() << ", " << status.store_status() << ", "
             << status.validation_status() << ", " << status.lock_status();
  // TODO(mukai): bring some error message on the screen.
  // https://crbug.com/835898
  NOTIMPLEMENTED();
}

void DemoSetupScreen::OnOtherError(
    EnterpriseEnrollmentHelper::OtherError error) {
  LOG(ERROR) << "Other error: " << error;
  // TODO(mukai): bring some error message on the screen.
  // https://crbug.com/835898
  NOTIMPLEMENTED();
}

void DemoSetupScreen::OnDeviceEnrolled(const std::string& additional_token) {
  NOTIMPLEMENTED();
}

void DemoSetupScreen::OnDeviceAttributeUpdatePermission(bool granted) {
  NOTREACHED();
}

void DemoSetupScreen::OnDeviceAttributeUploadCompleted(bool success) {
  NOTREACHED();
}

void DemoSetupScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionOnlineSetup) {
    NOTIMPLEMENTED();
  } else if (action_id == kUserActionOfflineSetup) {
    // TODO(mukai): load the policy data from somewhere (maybe asynchronously)
    // and then set the loaded policy data into config. https://crbug.com/827290
    policy::EnrollmentConfig config;
    config.mode = policy::EnrollmentConfig::MODE_OFFLINE_DEMO;
    config.management_domain = kDemoModeDomain;
    enrollment_helper_ = EnterpriseEnrollmentHelper::Create(
        this, nullptr /* ad_join_delegate */, config, kDemoModeDomain);
    enrollment_helper_->EnrollForOfflineDemo();
  } else if (action_id == kUserActionClose) {
    Finish(ScreenExitCode::DEMO_MODE_SETUP_CLOSED);
  } else {
    BaseScreen::OnUserAction(action_id);
  }
}

void DemoSetupScreen::OnViewDestroyed(DemoSetupScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
  enrollment_helper_.reset();
}

}  // namespace chromeos
