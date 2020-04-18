// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_KIOSK_ENABLE_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_KIOSK_ENABLE_SCREEN_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/login/screens/kiosk_enable_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

// WebUI implementation of KioskEnableScreenActor.
class KioskEnableScreenHandler : public KioskEnableScreenView,
                                 public BaseScreenHandler {
 public:
  KioskEnableScreenHandler();
  ~KioskEnableScreenHandler() override;

  // KioskEnableScreenActor implementation:
  void Show() override;
  void SetDelegate(Delegate* delegate) override;

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

 private:
  // JS messages handlers.
  void HandleOnClose();
  void HandleOnEnable();

  // Callback for KioskAppManager::EnableConsumerModeKiosk().
  void OnEnableConsumerKioskAutoLaunch(bool success);

  // Callback for KioskAppManager::GetConsumerKioskModeStatus().
  void OnGetConsumerKioskAutoLaunchStatus(
      KioskAppManager::ConsumerKioskAutoLaunchStatus status);

  Delegate* delegate_ = nullptr;

  // Keeps whether screen should be shown right after initialization.
  bool show_on_init_ = false;

  // True if machine's consumer kiosk mode is in a configurable state.
  bool is_configurable_ = false;

  base::WeakPtrFactory<KioskEnableScreenHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(KioskEnableScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_KIOSK_ENABLE_SCREEN_HANDLER_H_
