// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/gaia_cookie_manager_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "google_apis/gaia/gaia_constants.h"

GaiaCookieManagerServiceFactory::GaiaCookieManagerServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "GaiaCookieManagerService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ChromeSigninClientFactory::GetInstance());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
}

GaiaCookieManagerServiceFactory::~GaiaCookieManagerServiceFactory() {}

// static
GaiaCookieManagerService* GaiaCookieManagerServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<GaiaCookieManagerService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
GaiaCookieManagerServiceFactory*
GaiaCookieManagerServiceFactory::GetInstance() {
  return base::Singleton<GaiaCookieManagerServiceFactory>::get();
}

KeyedService* GaiaCookieManagerServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  GaiaCookieManagerService* cookie_service = new GaiaCookieManagerService(
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      GaiaConstants::kChromeSource,
      ChromeSigninClientFactory::GetForProfile(profile));
  return cookie_service;
}

void GaiaCookieManagerServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
}
