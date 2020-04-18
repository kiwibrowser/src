// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_EASY_UNLOCK_SETTINGS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_EASY_UNLOCK_SETTINGS_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_service_observer.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

namespace content {
class WebUIDataSource;
}

class Profile;

namespace chromeos {
namespace settings {

class EasyUnlockSettingsHandler : public ::settings::SettingsPageUIHandler,
                                  public EasyUnlockServiceObserver {
 public:
  // Returns nullptr if EasyUnlock is not allowed for this device.
  static EasyUnlockSettingsHandler* Create(
      content::WebUIDataSource* html_source,
      Profile* profile);

  ~EasyUnlockSettingsHandler() override;

  // SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // EasyUnlockServiceObserver:
  void OnTurnOffOperationStatusChanged() override;

 protected:
  explicit EasyUnlockSettingsHandler(Profile* profile);

 private:
  FRIEND_TEST_ALL_PREFIXES(EasyUnlockSettingsHandlerTest, EnabledStatus);
  FRIEND_TEST_ALL_PREFIXES(EasyUnlockSettingsHandlerTest, TurnOffFlowStatus);

  void SendEnabledStatus();
  std::string GetTurnOffFlowStatus();

  // JS callbacks.
  void HandleGetEnabledStatus(const base::ListValue* args);
  void HandleStartTurnOnFlow(const base::ListValue* args);
  void HandleGetTurnOffFlowStatus(const base::ListValue* args);
  void HandleStartTurnOffFlow(const base::ListValue* args);
  void HandleCancelTurnOffFlow(const base::ListValue* args);

  Profile* const profile_;

  PrefChangeRegistrar profile_pref_registrar_;

  DISALLOW_COPY_AND_ASSIGN(EasyUnlockSettingsHandler);
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_EASY_UNLOCK_SETTINGS_HANDLER_H_
