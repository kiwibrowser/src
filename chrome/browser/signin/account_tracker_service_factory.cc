// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_tracker_service_factory.h"

#include <memory>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/core/browser/account_tracker_service.h"

AccountTrackerServiceFactory::AccountTrackerServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "AccountTrackerServiceFactory",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ChromeSigninClientFactory::GetInstance());
}

AccountTrackerServiceFactory::~AccountTrackerServiceFactory() {
}

// static
AccountTrackerService*
AccountTrackerServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<AccountTrackerService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AccountTrackerServiceFactory* AccountTrackerServiceFactory::GetInstance() {
  return base::Singleton<AccountTrackerServiceFactory>::get();
}

void AccountTrackerServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  AccountTrackerService::RegisterPrefs(registry);
}

KeyedService* AccountTrackerServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  AccountTrackerService* service = new AccountTrackerService();
  service->Initialize(ChromeSigninClientFactory::GetForProfile(profile),
                      profile->GetPath());
  return service;
}
