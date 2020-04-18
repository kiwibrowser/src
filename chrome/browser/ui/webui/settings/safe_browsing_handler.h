// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_SAFE_BROWSING_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_SAFE_BROWSING_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

namespace settings {

class SafeBrowsingHandler : public SettingsPageUIHandler {
 public:
  explicit SafeBrowsingHandler(PrefService* prefs);
  ~SafeBrowsingHandler() override;

  // SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  // Handler for "getSafeBrowsingExtendedReporting" message. Passed a single
  // callback ID argument.
  void HandleGetSafeBrowsingExtendedReporting(const base::ListValue* args);

  // Handler for "setSafeBrowsingExtendedReportingEnabled" message. Passed a
  // single |enabled| boolean argument.
  void HandleSetSafeBrowsingExtendedReportingEnabled(
      const base::ListValue* args);

  // Called when the local state pref controlling Safe Browsing extended
  // reporting changes.
  void OnPrefChanged(const std::string& pref_name);

  // Used to track pref changes that affect whether Safe Browsing extended
  // reporting is enabled.
  PrefChangeRegistrar profile_pref_registrar_;

  // Weak pointer.
  PrefService* prefs_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_SAFE_BROWSING_HANDLER_H_
