// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_USER_IMAGE_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_USER_IMAGE_VIEW_H_

#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

class UserImageScreen;

// Interface for dependency injection between UserImageScreen and its actual
// representation, either views based or WebUI.
class UserImageView {
 public:
  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_USER_IMAGE_PICKER;

  virtual ~UserImageView() {}

  virtual void Bind(UserImageScreen* screen) = 0;

  virtual void Unbind() = 0;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  // Hides the contents of the screen.
  virtual void Hide() = 0;

  // Hides curtain with spinner.
  virtual void HideCurtain() = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_USER_IMAGE_VIEW_H_
