// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_WAIT_FOR_CONTAINER_READY_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_WAIT_FOR_CONTAINER_READY_SCREEN_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/login/screens/wait_for_container_ready_screen_view.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class WaitForContainerReadyScreenHandler
    : public BaseScreenHandler,
      public WaitForContainerReadyScreenView,
      public ArcAppListPrefs::Observer,
      public arc::ArcSessionManager::Observer {
 public:
  WaitForContainerReadyScreenHandler();
  ~WaitForContainerReadyScreenHandler() override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;

  // WaitForContainerReadyScreenView:
  void Bind(WaitForContainerReadyScreen* screen) override;
  void Unbind() override;
  void Show() override;
  void Hide() override;

  // ArcAppListPrefs::Observer overrides.
  void OnPackageListInitialRefreshed() override;

  // ArcSessionManager::Observer overrides.
  void OnArcErrorShowRequested(ArcSupportHost::Error error) override;

 private:
  // BaseScreenHandler:
  void Initialize() override;

  // Called to notify the screen that the container is ready.
  void NotifyContainerReady();

  // Called when the max wait timeout is reached.
  void OnMaxContainerWaitTimeout();

  WaitForContainerReadyScreen* screen_ = nullptr;

  // Whether the screen should be shown right after initialization.
  bool show_on_init_ = false;

  // Whether app list is ready.
  bool is_app_list_ready_ = false;

  // The primary user profile.
  Profile* profile_ = nullptr;

  // Timer used to exit the page when timeout reaches.
  base::OneShotTimer timer_;

  base::WeakPtrFactory<WaitForContainerReadyScreenHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WaitForContainerReadyScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_WAIT_FOR_CONTAINER_READY_SCREEN_HANDLER_H_
