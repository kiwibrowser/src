// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/gaia_info_update_service_factory.h"

#include "chrome/browser/profiles/gaia_info_update_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

GAIAInfoUpdateServiceFactory::GAIAInfoUpdateServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "GAIAInfoUpdateService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninManagerFactory::GetInstance());
}

GAIAInfoUpdateServiceFactory::~GAIAInfoUpdateServiceFactory() {}

// static
GAIAInfoUpdateService* GAIAInfoUpdateServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<GAIAInfoUpdateService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
GAIAInfoUpdateServiceFactory* GAIAInfoUpdateServiceFactory::GetInstance() {
  return base::Singleton<GAIAInfoUpdateServiceFactory>::get();
}

KeyedService* GAIAInfoUpdateServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  if (!GAIAInfoUpdateService::ShouldUseGAIAProfileInfo(profile))
    return NULL;
  return new GAIAInfoUpdateService(profile);
}

void GAIAInfoUpdateServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* prefs) {
  prefs->RegisterInt64Pref(prefs::kProfileGAIAInfoUpdateTime, 0);
  prefs->RegisterStringPref(prefs::kProfileGAIAInfoPictureURL, std::string());
}

bool GAIAInfoUpdateServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
