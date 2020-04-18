// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_MD_USER_MANAGER_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_MD_USER_MANAGER_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class SigninCreateProfileHandler;
class UserManagerScreenHandler;

namespace base {
class DictionaryValue;
}
namespace content {
class WebUIDataSource;
}

// A WebUI dialog to display available users.
class MDUserManagerUI : public content::WebUIController {
 public:
  explicit MDUserManagerUI(content::WebUI* web_ui);
  ~MDUserManagerUI() override;

 private:
  content::WebUIDataSource* CreateUIDataSource(
      const base::DictionaryValue& localized_strings);
  void GetLocalizedStrings(base::DictionaryValue* localized_strings);

  SigninCreateProfileHandler* signin_create_profile_handler_ = nullptr;
  UserManagerScreenHandler* user_manager_screen_handler_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(MDUserManagerUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_MD_USER_MANAGER_UI_H_
