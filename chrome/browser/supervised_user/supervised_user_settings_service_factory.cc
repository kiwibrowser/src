// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_settings_service_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
SupervisedUserSettingsService*
SupervisedUserSettingsServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<SupervisedUserSettingsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SupervisedUserSettingsServiceFactory*
SupervisedUserSettingsServiceFactory::GetInstance() {
  return base::Singleton<SupervisedUserSettingsServiceFactory>::get();
}

SupervisedUserSettingsServiceFactory::SupervisedUserSettingsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SupervisedUserSettingsService",
          BrowserContextDependencyManager::GetInstance()) {
}

SupervisedUserSettingsServiceFactory::
    ~SupervisedUserSettingsServiceFactory() {}

KeyedService* SupervisedUserSettingsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new SupervisedUserSettingsService(
      Profile::FromBrowserContext(profile));
}

content::BrowserContext*
SupervisedUserSettingsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
