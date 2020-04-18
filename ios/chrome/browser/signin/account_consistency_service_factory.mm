// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/account_consistency_service_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/ios/browser/account_consistency_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/content_settings/cookie_settings_factory.h"
#include "ios/chrome/browser/signin/account_reconcilor_factory.h"
#include "ios/chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios {

AccountConsistencyServiceFactory::AccountConsistencyServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AccountConsistencyService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::AccountReconcilorFactory::GetInstance());
  DependsOn(ios::CookieSettingsFactory::GetInstance());
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(SigninClientFactory::GetInstance());
  DependsOn(ios::SigninManagerFactory::GetInstance());
}

AccountConsistencyServiceFactory::~AccountConsistencyServiceFactory() {}

// static
AccountConsistencyService* AccountConsistencyServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<AccountConsistencyService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AccountConsistencyServiceFactory*
AccountConsistencyServiceFactory::GetInstance() {
  return base::Singleton<AccountConsistencyServiceFactory>::get();
}

void AccountConsistencyServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AccountConsistencyService::RegisterPrefs(registry);
}

std::unique_ptr<KeyedService>
AccountConsistencyServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<AccountConsistencyService>(
      chrome_browser_state,
      ios::AccountReconcilorFactory::GetForBrowserState(chrome_browser_state),
      ios::CookieSettingsFactory::GetForBrowserState(chrome_browser_state),
      GaiaCookieManagerServiceFactory::GetForBrowserState(chrome_browser_state),
      SigninClientFactory::GetForBrowserState(chrome_browser_state),
      ios::SigninManagerFactory::GetForBrowserState(chrome_browser_state));
}

}  // namespace ios
