// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/app_launch_utils.h"

#include "base/macros.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_launch_error.h"
#include "chrome/browser/chromeos/app_mode/startup_app_launcher.h"
#include "chrome/browser/lifetime/application_lifetime.h"

namespace chromeos {

// A simple manager for the app launch that starts the launch
// and deletes itself when the launch finishes. On launch failure,
// it exits the browser process.
class AppLaunchManager : public StartupAppLauncher::Delegate {
 public:
  AppLaunchManager(Profile* profile, const std::string& app_id)
      : startup_app_launcher_(
            new StartupAppLauncher(profile,
                                   app_id,
                                   false /* diagnostic_mode */,
                                   this)) {}

  void Start() {
    startup_app_launcher_->Initialize();
  }

 private:
  ~AppLaunchManager() override {}

  void Cleanup() { delete this; }

  // StartupAppLauncher::Delegate overrides:
  void InitializeNetwork() override {
    // This is on crash-restart path and assumes network is online.
    // TODO(xiyuan): Remove the crash-restart path for kiosk or add proper
    // network configure handling.
    startup_app_launcher_->ContinueWithNetworkReady();
  }
  bool IsNetworkReady() override {
    // See comments above. Network is assumed to be online here.
    return true;
  }
  bool ShouldSkipAppInstallation() override {
    // Given that this delegate does not reliably report whether the network is
    // ready, avoid making app update checks - this might take a while if
    // network is not online. Also, during crash-restart, we should continue
    // with the same app version as the restored session.
    return true;
  }
  void OnInstallingApp() override {}
  void OnReadyToLaunch() override { startup_app_launcher_->LaunchApp(); }
  void OnLaunchSucceeded() override { Cleanup(); }
  void OnLaunchFailed(KioskAppLaunchError::Error error) override {
    KioskAppLaunchError::Save(error);
    chrome::AttemptUserExit();
    Cleanup();
  }
  bool IsShowingNetworkConfigScreen() override { return false; }

  std::unique_ptr<StartupAppLauncher> startup_app_launcher_;

  DISALLOW_COPY_AND_ASSIGN(AppLaunchManager);
};

void LaunchAppOrDie(Profile* profile, const std::string& app_id) {
  // AppLaunchManager manages its own lifetime.
  (new AppLaunchManager(profile, app_id))->Start();
}

}  // namespace chromeos
