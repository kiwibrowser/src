// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/device_disabled_screen.h"

#include <string>

#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"

namespace chromeos {

DeviceDisabledScreen::DeviceDisabledScreen(
    BaseScreenDelegate* base_screen_delegate,
    DeviceDisabledScreenView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_DEVICE_DISABLED),
      view_(view),
      device_disabling_manager_(
          g_browser_process->platform_part()->device_disabling_manager()),
      showing_(false) {
  view_->SetDelegate(this);
  device_disabling_manager_->AddObserver(this);
}

DeviceDisabledScreen::~DeviceDisabledScreen() {
  if (view_)
    view_->SetDelegate(nullptr);
  device_disabling_manager_->RemoveObserver(this);
}

void DeviceDisabledScreen::Show() {
  if (!view_ || showing_)
    return;

  showing_ = true;
  view_->Show();
}

void DeviceDisabledScreen::Hide() {
  if (!showing_)
    return;
  showing_ = false;

  if (view_)
    view_->Hide();
}

void DeviceDisabledScreen::OnViewDestroyed(DeviceDisabledScreenView* view) {
  if (view_ == view)
    view_ = nullptr;
}

const std::string& DeviceDisabledScreen::GetEnrollmentDomain() const {
  return device_disabling_manager_->enrollment_domain();
}

const std::string& DeviceDisabledScreen::GetMessage() const {
  return device_disabling_manager_->disabled_message();
}

void DeviceDisabledScreen::OnDisabledMessageChanged(
    const std::string& disabled_message) {
  if (view_)
    view_->UpdateMessage(disabled_message);
}

}  // namespace chromeos
