// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEVICE_DISABLED_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEVICE_DISABLED_SCREEN_VIEW_H_

#include <string>
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

// Interface between the device disabled screen and its representation.
class DeviceDisabledScreenView {
 public:
  // Allows the representation to access information about the screen.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when the view is being destroyed. Note that if the Delegate is
    // destroyed first, it must call SetDelegate(nullptr).
    virtual void OnViewDestroyed(DeviceDisabledScreenView* view) = 0;

    // Returns the domain that owns the device.
    virtual const std::string& GetEnrollmentDomain() const = 0;

    // Returns the message that should be shown to the user.
    virtual const std::string& GetMessage() const = 0;
  };

  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_DEVICE_DISABLED;

  virtual ~DeviceDisabledScreenView() {}

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void SetDelegate(Delegate* delegate) = 0;
  virtual void UpdateMessage(const std::string& message) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_DEVICE_DISABLED_SCREEN_VIEW_H_
