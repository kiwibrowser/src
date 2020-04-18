// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_KIOSK_SPLASH_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_KIOSK_SPLASH_SCREEN_VIEW_H_

#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

// Interface for UI implemenations of the ArcKioskSplashScreen.
class ArcKioskSplashScreenView {
 public:
  enum class ArcKioskState {
    STARTING_SESSION,
    WAITING_APP_LAUNCH,
    WAITING_APP_WINDOW,
  };

  class Delegate {
   public:
    Delegate() = default;
    // Invoked when the launch bailout shortcut key is pressed.
    virtual void OnCancelArcKioskLaunch() = 0;

    // Invoked when the splash screen view gets being deleted.
    virtual void OnDeletingSplashScreenView() = 0;

   protected:
    virtual ~Delegate() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_ARC_KIOSK_SPLASH;

  ArcKioskSplashScreenView() = default;

  virtual ~ArcKioskSplashScreenView() = default;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  // Set the current ARC kiosk state.
  virtual void UpdateArcKioskState(ArcKioskState state) = 0;

  // Sets screen this view belongs to.
  virtual void SetDelegate(Delegate* delegate) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcKioskSplashScreenView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_ARC_KIOSK_SPLASH_SCREEN_VIEW_H_
