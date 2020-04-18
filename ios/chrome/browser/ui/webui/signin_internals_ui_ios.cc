// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/signin_internals_ui_ios.h"

#include "base/hash.h"
#include "components/grit/components_resources.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/signin/about_signin_internals_factory.h"
#include "ios/chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "ios/web/public/web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios.h"

namespace {

web::WebUIIOSDataSource* CreateSignInInternalsHTMLSource() {
  web::WebUIIOSDataSource* source =
      web::WebUIIOSDataSource::Create(kChromeUISignInInternalsHost);

  source->SetJsonPath("strings.js");
  source->AddResourcePath("signin_internals.js", IDR_SIGNIN_INTERNALS_INDEX_JS);
  source->SetDefaultResource(IDR_SIGNIN_INTERNALS_INDEX_HTML);
  source->UseGzip();

  return source;
}

}  //  namespace

SignInInternalsUIIOS::SignInInternalsUIIOS(web::WebUIIOS* web_ui)
    : WebUIIOSController(web_ui) {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui);
  DCHECK(browser_state);
  web::WebUIIOSDataSource::Add(browser_state,
                               CreateSignInInternalsHTMLSource());

  AboutSigninInternals* about_signin_internals =
      ios::AboutSigninInternalsFactory::GetForBrowserState(browser_state);
  if (about_signin_internals)
    about_signin_internals->AddSigninObserver(this);
}

SignInInternalsUIIOS::~SignInInternalsUIIOS() {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui());
  DCHECK(browser_state);
  AboutSigninInternals* about_signin_internals =
      ios::AboutSigninInternalsFactory::GetForBrowserState(browser_state);
  if (about_signin_internals)
    about_signin_internals->RemoveSigninObserver(this);
}

bool SignInInternalsUIIOS::OverrideHandleWebUIIOSMessage(
    const GURL& source_url,
    const std::string& name,
    const base::ListValue& content) {
  if (name == "getSigninInfo") {
    ios::ChromeBrowserState* browser_state =
        ios::ChromeBrowserState::FromWebUIIOS(web_ui());
    DCHECK(browser_state);

    AboutSigninInternals* about_signin_internals =
        ios::AboutSigninInternalsFactory::GetForBrowserState(browser_state);
    // TODO(vishwath): The UI would look better if we passed in a dict with some
    // reasonable defaults, so the about:signin-internals page doesn't look
    // empty in incognito mode. Alternatively, we could force about:signin to
    // open in non-incognito mode always (like about:settings for ex.).
    if (about_signin_internals) {
      const std::string& reply_handler =
          "chrome.signin.getSigninInfo.handleReply";
      web_ui()->CallJavascriptFunction(
          reply_handler, *about_signin_internals->GetSigninStatus());
      std::vector<gaia::ListedAccount> cookie_accounts;
      GaiaCookieManagerService* cookie_manager_service =
          ios::GaiaCookieManagerServiceFactory::GetForBrowserState(
              browser_state);
      std::vector<gaia::ListedAccount> signed_out_accounts;
      if (cookie_manager_service->ListAccounts(
              &cookie_accounts, &signed_out_accounts,
              "ChromiumSignInInternalsUIIOS")) {
        about_signin_internals->OnGaiaAccountsInCookieUpdated(
            cookie_accounts, signed_out_accounts,
            GoogleServiceAuthError(GoogleServiceAuthError::NONE));
      }

      return true;
    }
  }
  return false;
}

void SignInInternalsUIIOS::OnSigninStateChanged(
    const base::DictionaryValue* info) {
  const std::string& event_handler = "chrome.signin.onSigninInfoChanged.fire";
  web_ui()->CallJavascriptFunction(event_handler, *info);
}

void SignInInternalsUIIOS::OnCookieAccountsFetched(
    const base::DictionaryValue* info) {
  web_ui()->CallJavascriptFunction("chrome.signin.onCookieAccountsFetched.fire",
                                   *info);
}
