// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHANGE_PASSWORD_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHANGE_PASSWORD_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

namespace safe_browsing {
class ChromePasswordProtectionService;
}

namespace settings {

// Chrome "Change Password" settings page UI handler.
class ChangePasswordHandler : public SettingsPageUIHandler {
 public:
  explicit ChangePasswordHandler(
      Profile* profile,
      safe_browsing::ChromePasswordProtectionService* service);
  ~ChangePasswordHandler() override;

  // settings::SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  void HandleInitialize(const base::ListValue* args);

  void HandleChangePassword(const base::ListValue* args);

  void UpdateChangePasswordCardVisibility();

  Profile* profile_;

  PrefChangeRegistrar pref_registrar_;

  safe_browsing::ChromePasswordProtectionService* service_;

  DISALLOW_COPY_AND_ASSIGN(ChangePasswordHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHANGE_PASSWORD_HANDLER_H_
