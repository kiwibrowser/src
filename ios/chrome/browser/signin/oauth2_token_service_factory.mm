// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/ios/browser/profile_oauth2_token_service_ios_delegate.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/account_tracker_service_factory.h"
#include "ios/chrome/browser/signin/profile_oauth2_token_service_ios_provider_impl.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"
#include "ios/chrome/browser/signin/signin_error_controller_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OAuth2TokenServiceFactory::OAuth2TokenServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "ProfileOAuth2TokenService",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::AccountTrackerServiceFactory::GetInstance());
  DependsOn(SigninClientFactory::GetInstance());
  DependsOn(ios::SigninErrorControllerFactory::GetInstance());
}

OAuth2TokenServiceFactory::~OAuth2TokenServiceFactory() {}

ProfileOAuth2TokenService* OAuth2TokenServiceFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<ProfileOAuth2TokenService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
OAuth2TokenServiceFactory* OAuth2TokenServiceFactory::GetInstance() {
  return base::Singleton<OAuth2TokenServiceFactory>::get();
}

void OAuth2TokenServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  ProfileOAuth2TokenService::RegisterProfilePrefs(registry);
}

std::unique_ptr<KeyedService>
OAuth2TokenServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  auto delegate = std::make_unique<ProfileOAuth2TokenServiceIOSDelegate>(
      SigninClientFactory::GetForBrowserState(chrome_browser_state),
      std::make_unique<ProfileOAuth2TokenServiceIOSProviderImpl>(),
      ios::AccountTrackerServiceFactory::GetForBrowserState(
          chrome_browser_state),
      ios::SigninErrorControllerFactory::GetForBrowserState(
          chrome_browser_state));
  return std::make_unique<ProfileOAuth2TokenService>(std::move(delegate));
}
