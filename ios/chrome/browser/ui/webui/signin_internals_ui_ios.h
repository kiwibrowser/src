// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_SIGNIN_INTERNALS_UI_IOS_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_SIGNIN_INTERNALS_UI_IOS_H_

#include "base/macros.h"
#include "base/values.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

// The implementation for the chrome://signin-internals page.
class SignInInternalsUIIOS : public web::WebUIIOSController,
                             public AboutSigninInternals::Observer {
 public:
  explicit SignInInternalsUIIOS(web::WebUIIOS* web_ui);
  ~SignInInternalsUIIOS() override;

  // web::WebUIIOSController implementation.
  bool OverrideHandleWebUIIOSMessage(const GURL& source_url,
                                     const std::string& name,
                                     const base::ListValue& args) override;

  // AboutSigninInternals::Observer::OnSigninStateChanged implementation.
  void OnSigninStateChanged(const base::DictionaryValue* info) override;

  // Notification that the cookie accounts are ready to be displayed.
  void OnCookieAccountsFetched(const base::DictionaryValue* info) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SignInInternalsUIIOS);
};

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_SIGNIN_INTERNALS_UI_IOS_H_
