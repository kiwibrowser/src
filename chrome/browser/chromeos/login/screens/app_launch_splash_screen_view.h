// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_APP_LAUNCH_SPLASH_SCREEN_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_APP_LAUNCH_SPLASH_SCREEN_VIEW_H_

#include "base/strings/string16.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"

namespace chromeos {

// Interface for UI implemenations of the ApplaunchSplashScreen.
class AppLaunchSplashScreenView {
 public:
  enum AppLaunchState {
    APP_LAUNCH_STATE_PREPARING_NETWORK,
    APP_LAUNCH_STATE_INSTALLING_APPLICATION,
    APP_LAUNCH_STATE_WAITING_APP_WINDOW,
    APP_LAUNCH_STATE_NETWORK_WAIT_TIMEOUT,
    APP_LAUNCH_STATE_SHOWING_NETWORK_CONFIGURE_UI,
  };

  class Delegate {
   public:
    // Invoked when the configure network control is clicked.
    virtual void OnConfigureNetwork() = 0;

    // Invoked when the app launch bailout shortcut key is pressed.
    virtual void OnCancelAppLaunch() = 0;

    // Invoked when the network config shortcut key is pressed.
    virtual void OnNetworkConfigRequested(bool requested) = 0;

    // Invoked when network state is changed. |online| is true if the device
    // is connected to the Internet.
    virtual void OnNetworkStateChanged(bool online) = 0;

    // Invoked when the splash screen view is being deleted.
    virtual void OnDeletingSplashScreenView() = 0;

   protected:
    virtual ~Delegate() {}
  };

  constexpr static OobeScreen kScreenId = OobeScreen::SCREEN_APP_LAUNCH_SPLASH;

  virtual ~AppLaunchSplashScreenView() {}

  // Sets screen this view belongs to.
  virtual void SetDelegate(Delegate* screen) = 0;

  // Shows the contents of the screen.
  virtual void Show(const std::string& app_id) = 0;

  // Hides the contents of the screen.
  virtual void Hide() = 0;

  // Set the current app launch state.
  virtual void UpdateAppLaunchState(AppLaunchState state) = 0;

  // Sets whether configure network control is visible.
  virtual void ToggleNetworkConfig(bool visible) = 0;

  // Shows the network error and configure UI.
  virtual void ShowNetworkConfigureUI() = 0;

  // Returns true if the default network has Internet access.
  virtual bool IsNetworkReady() = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_APP_LAUNCH_SPLASH_SCREEN_VIEW_H_
