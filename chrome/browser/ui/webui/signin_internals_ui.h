// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_INTERNALS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_INTERNALS_UI_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "content/public/browser/web_ui_controller.h"

// The implementation for the chrome://signin-internals page.
class SignInInternalsUI : public content::WebUIController,
                          public AboutSigninInternals::Observer {
 public:
  explicit SignInInternalsUI(content::WebUI* web_ui);
  ~SignInInternalsUI() override;

  // content::WebUIController implementation.
  bool OverrideHandleWebUIMessage(const GURL& source_url,
                                  const std::string& name,
                                  const base::ListValue& args) override;

  // AboutSigninInternals::Observer::OnSigninStateChanged implementation.
  void OnSigninStateChanged(const base::DictionaryValue* info) override;

  // Notification that the cookie accounts are ready to be displayed.
  void OnCookieAccountsFetched(const base::DictionaryValue* info) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SignInInternalsUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_INTERNALS_UI_H_
