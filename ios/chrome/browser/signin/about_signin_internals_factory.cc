// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/about_signin_internals_factory.h"

#include <utility>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#include "ios/chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"
#include "ios/chrome/browser/signin/signin_error_controller_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"

namespace ios {

AboutSigninInternalsFactory::AboutSigninInternalsFactory()
    : BrowserStateKeyedServiceFactory(
          "AboutSigninInternals",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(AccountTrackerServiceFactory::GetInstance());
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(OAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninClientFactory::GetInstance());
  DependsOn(SigninErrorControllerFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
}

AboutSigninInternalsFactory::~AboutSigninInternalsFactory() {}

// static
AboutSigninInternals* AboutSigninInternalsFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<AboutSigninInternals*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AboutSigninInternalsFactory* AboutSigninInternalsFactory::GetInstance() {
  return base::Singleton<AboutSigninInternalsFactory>::get();
}

std::unique_ptr<KeyedService>
AboutSigninInternalsFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<AboutSigninInternals> service(new AboutSigninInternals(
      OAuth2TokenServiceFactory::GetForBrowserState(chrome_browser_state),
      AccountTrackerServiceFactory::GetForBrowserState(chrome_browser_state),
      SigninManagerFactory::GetForBrowserState(chrome_browser_state),
      SigninErrorControllerFactory::GetForBrowserState(chrome_browser_state),
      GaiaCookieManagerServiceFactory::GetForBrowserState(chrome_browser_state),
      signin::AccountConsistencyMethod::kMirror));
  service->Initialize(
      SigninClientFactory::GetForBrowserState(chrome_browser_state));
  return service;
}

void AboutSigninInternalsFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {
  AboutSigninInternals::RegisterPrefs(user_prefs);
}

}  // namespace ios
