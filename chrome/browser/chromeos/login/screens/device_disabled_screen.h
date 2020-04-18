// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEVICE_DISABLED_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEVICE_DISABLED_SCREEN_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/device_disabled_screen_view.h"
#include "chrome/browser/chromeos/system/device_disabling_manager.h"

namespace chromeos {

namespace system {
class DeviceDisablingManager;
}

class BaseScreenDelegate;

// Screen informing the user that the device has been disabled by its owner.
class DeviceDisabledScreen : public BaseScreen,
                             public DeviceDisabledScreenView::Delegate,
                             public system::DeviceDisablingManager::Observer {
 public:
  DeviceDisabledScreen(BaseScreenDelegate* base_screen_delegate,
                       DeviceDisabledScreenView* view);
  ~DeviceDisabledScreen() override;

  // BaseScreen:
  void Show() override;
  void Hide() override;

  // DeviceDisabledScreenView::Delegate:
  void OnViewDestroyed(DeviceDisabledScreenView* view) override;
  const std::string& GetEnrollmentDomain() const override;
  const std::string& GetMessage() const override;

  // system::DeviceDisablingManager::Observer:
  void OnDisabledMessageChanged(const std::string& disabled_message) override;

 private:
  DeviceDisabledScreenView* view_;
  system::DeviceDisablingManager* device_disabling_manager_;

  // Whether the screen is currently showing.
  bool showing_;

  DISALLOW_COPY_AND_ASSIGN(DeviceDisabledScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEVICE_DISABLED_SCREEN_H_
