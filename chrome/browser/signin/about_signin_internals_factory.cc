// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/about_signin_internals_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/signin_manager.h"

AboutSigninInternalsFactory::AboutSigninInternalsFactory()
    : BrowserContextKeyedServiceFactory(
        "AboutSigninInternals",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(AccountTrackerServiceFactory::GetInstance());
  DependsOn(ChromeSigninClientFactory::GetInstance());
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninErrorControllerFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
}

AboutSigninInternalsFactory::~AboutSigninInternalsFactory() {}

// static
AboutSigninInternals* AboutSigninInternalsFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AboutSigninInternals*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AboutSigninInternalsFactory* AboutSigninInternalsFactory::GetInstance() {
  return base::Singleton<AboutSigninInternalsFactory>::get();
}

void AboutSigninInternalsFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {
  AboutSigninInternals::RegisterPrefs(user_prefs);
}

KeyedService* AboutSigninInternalsFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  AboutSigninInternals* service = new AboutSigninInternals(
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      AccountTrackerServiceFactory::GetForProfile(profile),
      SigninManagerFactory::GetForProfile(profile),
      SigninErrorControllerFactory::GetForProfile(profile),
      GaiaCookieManagerServiceFactory::GetForProfile(profile),
      AccountConsistencyModeManager::GetMethodForProfile(profile));
  service->Initialize(ChromeSigninClientFactory::GetForProfile(profile));
  return service;
}
