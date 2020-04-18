// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/cryptauth/chrome_cryptauth_service_factory.h"

#include "chrome/browser/chromeos/cryptauth/chrome_cryptauth_service.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace chromeos {

// static
cryptauth::CryptAuthService*
ChromeCryptAuthServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ChromeCryptAuthService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ChromeCryptAuthServiceFactory* ChromeCryptAuthServiceFactory::GetInstance() {
  return base::Singleton<ChromeCryptAuthServiceFactory>::get();
}

ChromeCryptAuthServiceFactory::ChromeCryptAuthServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CryptAuthService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
  DependsOn(gcm::GCMProfileServiceFactory::GetInstance());
}

ChromeCryptAuthServiceFactory::~ChromeCryptAuthServiceFactory() {}

KeyedService* ChromeCryptAuthServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return ChromeCryptAuthService::Create(profile).release();
}

void ChromeCryptAuthServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  cryptauth::CryptAuthService::RegisterProfilePrefs(registry);
}

}  // namespace chromeos
