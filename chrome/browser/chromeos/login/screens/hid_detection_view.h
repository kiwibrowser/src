// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HID_DETECTION_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HID_DETECTION_VIEW_H_

#include <string>

#include "base/callback.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

class HIDDetectionScreen;

// Interface between HID detection screen and its representation, either WebUI
// or Views one. Note, do not forget to call OnViewDestroyed in the
// dtor.
class HIDDetectionView {
 public:
  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_OOBE_HID_DETECTION;

  virtual ~HIDDetectionView() {}

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void Bind(HIDDetectionScreen* screen) = 0;
  virtual void Unbind() = 0;
  // Checks if we should show the screen or enough devices already present.
  // Calls corresponding set of actions based on the bool result.
  virtual void CheckIsScreenRequired(
      const base::Callback<void(bool)>& on_check_done) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_HID_DETECTION_VIEW_H_
