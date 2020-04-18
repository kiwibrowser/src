// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin_internals_ui.h"

#include <memory>
#include <string>
#include <vector>

#include "base/hash.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/about_signin_internals_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/common/url_constants.h"
#include "components/grit/components_resources.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

content::WebUIDataSource* CreateSignInInternalsHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUISignInInternalsHost);

  source->SetJsonPath("strings.js");
  source->AddResourcePath("signin_internals.js", IDR_SIGNIN_INTERNALS_INDEX_JS);
  source->SetDefaultResource(IDR_SIGNIN_INTERNALS_INDEX_HTML);
  source->UseGzip();
  return source;
}

}  //  namespace

SignInInternalsUI::SignInInternalsUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateSignInInternalsHTMLSource());
  if (profile) {
    AboutSigninInternals* about_signin_internals =
        AboutSigninInternalsFactory::GetForProfile(profile);
    if (about_signin_internals)
      about_signin_internals->AddSigninObserver(this);
  }
}

SignInInternalsUI::~SignInInternalsUI() {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile) {
    AboutSigninInternals* about_signin_internals =
        AboutSigninInternalsFactory::GetForProfile(profile);
    if (about_signin_internals) {
      about_signin_internals->RemoveSigninObserver(this);
    }
  }
}

bool SignInInternalsUI::OverrideHandleWebUIMessage(
    const GURL& source_url,
    const std::string& name,
    const base::ListValue& content) {
  if (name == "getSigninInfo") {
    Profile* profile = Profile::FromWebUI(web_ui());
    if (!profile)
      return false;

    AboutSigninInternals* about_signin_internals =
        AboutSigninInternalsFactory::GetForProfile(profile);
    // TODO(vishwath): The UI would look better if we passed in a dict with some
    // reasonable defaults, so the about:signin-internals page doesn't look
    // empty in incognito mode. Alternatively, we could force about:signin to
    // open in non-incognito mode always (like about:settings for ex.).
    if (about_signin_internals) {
      web_ui()->CallJavascriptFunctionUnsafe(
          "chrome.signin.getSigninInfo.handleReply",
          *about_signin_internals->GetSigninStatus());

      std::vector<gaia::ListedAccount> cookie_accounts;
      std::vector<gaia::ListedAccount> signed_out_accounts;
      GaiaCookieManagerService* cookie_manager_service =
          GaiaCookieManagerServiceFactory::GetForProfile(profile);
      if (cookie_manager_service->ListAccounts(
              &cookie_accounts, &signed_out_accounts,
              "ChromiumSignInInternalsUI")) {
        about_signin_internals->OnGaiaAccountsInCookieUpdated(
            cookie_accounts,
            signed_out_accounts,
            GoogleServiceAuthError(GoogleServiceAuthError::NONE));
      }

      return true;
    }
  }
  return false;
}

void SignInInternalsUI::OnSigninStateChanged(
    const base::DictionaryValue* info) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "chrome.signin.onSigninInfoChanged.fire", *info);
}

void SignInInternalsUI::OnCookieAccountsFetched(
    const base::DictionaryValue* info) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "chrome.signin.onCookieAccountsFetched.fire", *info);
}
