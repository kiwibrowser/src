// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/auth_sync_observer_factory.h"

#include "chrome/browser/chromeos/login/signin/auth_sync_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace chromeos {

AuthSyncObserverFactory::AuthSyncObserverFactory()
    : BrowserContextKeyedServiceFactory(
          "AuthSyncObserver",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileSyncServiceFactory::GetInstance());
  DependsOn(SigninErrorControllerFactory::GetInstance());
}

AuthSyncObserverFactory::~AuthSyncObserverFactory() {}

// static
AuthSyncObserver* AuthSyncObserverFactory::GetForProfile(Profile* profile) {
  if (!AuthSyncObserver::ShouldObserve(profile))
    return nullptr;

  return static_cast<AuthSyncObserver*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AuthSyncObserverFactory* AuthSyncObserverFactory::GetInstance() {
  return base::Singleton<AuthSyncObserverFactory>::get();
}

KeyedService* AuthSyncObserverFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  return new AuthSyncObserver(profile);
}

}  // namespace chromeos
