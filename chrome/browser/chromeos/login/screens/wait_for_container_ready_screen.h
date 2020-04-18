// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WAIT_FOR_CONTAINER_READY_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WAIT_FOR_CONTAINER_READY_SCREEN_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"

namespace chromeos {

class WaitForContainerReadyScreenView;
class BaseScreenDelegate;

class WaitForContainerReadyScreen : public BaseScreen {
 public:
  WaitForContainerReadyScreen(BaseScreenDelegate* base_screen_delegate,
                              WaitForContainerReadyScreenView* view);
  ~WaitForContainerReadyScreen() override;

  // BaseScreen:
  void Show() override;
  void Hide() override;

  // Called when view is destroyed so there's no dead reference to it.
  void OnViewDestroyed(WaitForContainerReadyScreenView* view_);

  // Called when the container is ready, exit the screen.
  void OnContainerReady();

  // Called when error occurs or timeout, exit the screen.
  void OnContainerError();

 private:
  WaitForContainerReadyScreenView* view_;

  DISALLOW_COPY_AND_ASSIGN(WaitForContainerReadyScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_WAIT_FOR_CONTAINER_READY_SCREEN_H_
