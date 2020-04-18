// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/change_password_handler.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

namespace settings {

using safe_browsing::ChromePasswordProtectionService;

ChangePasswordHandler::ChangePasswordHandler(
    Profile* profile,
    safe_browsing::ChromePasswordProtectionService* service)
    : profile_(profile), service_(service) {
  DCHECK(service_);
}

ChangePasswordHandler::~ChangePasswordHandler() {}

void ChangePasswordHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "initializeChangePasswordHandler",
      base::BindRepeating(&ChangePasswordHandler::HandleInitialize,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "changePassword",
      base::BindRepeating(&ChangePasswordHandler::HandleChangePassword,
                          base::Unretained(this)));
}

void ChangePasswordHandler::OnJavascriptAllowed() {
  pref_registrar_.Init(profile_->GetPrefs());
  pref_registrar_.Add(
      prefs::kSafeBrowsingUnhandledSyncPasswordReuses,
      base::Bind(&ChangePasswordHandler::UpdateChangePasswordCardVisibility,
                 base::Unretained(this)));
}

void ChangePasswordHandler::OnJavascriptDisallowed() {
  pref_registrar_.RemoveAll();
}

void ChangePasswordHandler::HandleInitialize(const base::ListValue* args) {
    AllowJavascript();
    UpdateChangePasswordCardVisibility();
}

void ChangePasswordHandler::HandleChangePassword(const base::ListValue* args) {
  service_->OnUserAction(
      web_ui()->GetWebContents(),
      safe_browsing::PasswordProtectionService::CHROME_SETTINGS,
      safe_browsing::PasswordProtectionService::CHANGE_PASSWORD);
}

void ChangePasswordHandler::UpdateChangePasswordCardVisibility() {
  FireWebUIListener(
      "change-password-visibility",
      base::Value(service_->IsWarningEnabled() &&
                  safe_browsing::ChromePasswordProtectionService::
                      ShouldShowChangePasswordSettingUI(profile_)));
}

}  // namespace settings
