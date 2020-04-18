// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_LANGUAGES_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_LANGUAGES_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

namespace base {
class ListValue;
}

namespace content {
class WebUI;
}

class Profile;

namespace settings {

// Chrome "Languages" settings page UI handler.
class LanguagesHandler : public SettingsPageUIHandler {
 public:
  explicit LanguagesHandler(content::WebUI* webui);
  ~LanguagesHandler() override;

  // SettingsPageUIHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

 private:
  // Returns the prospective UI language. May not match the actual UI language,
  // depending on the user's permissions and whether the language is substituted
  // for another locale.
  void HandleGetProspectiveUILanguage(const base::ListValue* args);

  // Changes the preferred UI language, provided the user is allowed to do so.
  // The actual UI language will not change until the next restart.
  void HandleSetProspectiveUILanguage(const base::ListValue* args);

  Profile* profile_;  // Weak pointer.

  DISALLOW_COPY_AND_ASSIGN(LanguagesHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_LANGUAGES_HANDLER_H_
