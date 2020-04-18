// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WAIT_FOR_CONTAINER_READY_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WAIT_FOR_CONTAINER_READY_SCREEN_VIEW_H_

#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

class WaitForContainerReadyScreen;

// Interface for dependency injection between WaitForContainerReadyScreen
// and its WebUI representation.
class WaitForContainerReadyScreenView {
 public:
  constexpr static OobeScreen kScreenId =
      OobeScreen::SCREEN_WAIT_FOR_CONTAINER_READY;

  virtual ~WaitForContainerReadyScreenView() {}

  virtual void Bind(WaitForContainerReadyScreen* screen) = 0;
  virtual void Unbind() = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;

 protected:
  WaitForContainerReadyScreenView() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(WaitForContainerReadyScreenView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WAIT_FOR_CONTAINER_READY_SCREEN_VIEW_H_
