// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_KIOSK_APP_MENU_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_KIOSK_APP_MENU_HANDLER_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/app_mode/arc/arc_kiosk_app_manager.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager_observer.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/browser/ui/webui/chromeos/login/network_state_informer.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace chromeos {

// KioskAppMenuHandler supplies kiosk apps data to apps menu on sign-in
// screen when app mode is enabled and handles "launchKioskApp" request
// from the apps menu.
class KioskAppMenuHandler
    : public content::WebUIMessageHandler,
      public KioskAppManagerObserver,
      public NetworkStateInformer::NetworkStateInformerObserver,
      public ArcKioskAppManager::ArcKioskAppManagerObserver {
 public:
  explicit KioskAppMenuHandler(
      const scoped_refptr<NetworkStateInformer>& network_state_informer);
  ~KioskAppMenuHandler() override;

  void GetLocalizedStrings(base::DictionaryValue* localized_strings);

  // content::WebUIMessageHandler overrides:
  void RegisterMessages() override;

  // Returns true if new kiosk UI is enabled.
  static bool EnableNewKioskUI();

 private:
  // Sends all kiosk apps to webui.
  void SendKioskApps();

  // JS callbacks.
  void HandleInitializeKioskApps(const base::ListValue* args);
  void HandleKioskAppsLoaded(const base::ListValue* args);
  void HandleCheckKioskAppLaunchError(const base::ListValue* args);

  // KioskAppManagerObserver overrides:
  void OnKioskAppsSettingsChanged() override;
  void OnKioskAppDataChanged(const std::string& app_id) override;
  void OnKioskAppDataLoadFailure(const std::string& app_id) override;

  // NetworkStateInformer::NetworkStateInformerObserver overrides:
  void UpdateState(NetworkError::ErrorReason reason) override;

  // ArcKioskAppManager::ArcKioskAppManagerObserver overrides:
  void OnArcKioskAppsChanged() override;

  // True when WebUI is initialized. Otherwise don't allow calling JS functions.
  bool is_webui_initialized_;

  scoped_refptr<NetworkStateInformer> network_state_informer_;

  base::WeakPtrFactory<KioskAppMenuHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(KioskAppMenuHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_KIOSK_APP_MENU_HANDLER_H_
