// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/chrome_signin_client_factory.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "content/public/common/content_features.h"

ChromeSigninClientFactory::ChromeSigninClientFactory()
    : BrowserContextKeyedServiceFactory(
          "ChromeSigninClient",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninErrorControllerFactory::GetInstance());
  signin::SetGaiaOriginIsolatedCallback(base::Bind([] {
    return base::FeatureList::IsEnabled(features::kSignInProcessIsolation);
  }));
}

ChromeSigninClientFactory::~ChromeSigninClientFactory() {}

// static
SigninClient* ChromeSigninClientFactory::GetForProfile(Profile* profile) {
  return static_cast<SigninClient*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
ChromeSigninClientFactory* ChromeSigninClientFactory::GetInstance() {
  return base::Singleton<ChromeSigninClientFactory>::get();
}

KeyedService* ChromeSigninClientFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  ChromeSigninClient* client = new ChromeSigninClient(
      profile, SigninErrorControllerFactory::GetForProfile(profile));
  return client;
}

void ChromeSigninClientFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AccountConsistencyModeManager::RegisterProfilePrefs(registry);
}
