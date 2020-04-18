// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_manager.h"

LoginUIServiceFactory::LoginUIServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "LoginUIServiceFactory",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninManagerFactory::GetInstance());
  DependsOn(ProfileSyncServiceFactory::GetInstance());
}

LoginUIServiceFactory::~LoginUIServiceFactory() {}

// static
LoginUIService* LoginUIServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<LoginUIService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
LoginUIServiceFactory* LoginUIServiceFactory::GetInstance() {
  return base::Singleton<LoginUIServiceFactory>::get();
}

// static
base::Closure LoginUIServiceFactory::GetShowLoginPopupCallbackForProfile(
    Profile* profile) {
  return base::Bind(
      &LoginUIService::ShowLoginPopup,
      base::Unretained(LoginUIServiceFactory::GetForProfile(profile)));
}

KeyedService* LoginUIServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new LoginUIService(static_cast<Profile*>(profile));
}

bool LoginUIServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}
