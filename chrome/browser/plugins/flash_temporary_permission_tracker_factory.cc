// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/flash_temporary_permission_tracker_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
scoped_refptr<FlashTemporaryPermissionTracker>
FlashTemporaryPermissionTrackerFactory::GetForProfile(Profile* profile) {
  return static_cast<FlashTemporaryPermissionTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true).get());
}

// static
FlashTemporaryPermissionTrackerFactory*
FlashTemporaryPermissionTrackerFactory::GetInstance() {
  return base::Singleton<FlashTemporaryPermissionTrackerFactory>::get();
}

FlashTemporaryPermissionTrackerFactory::FlashTemporaryPermissionTrackerFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
          "FlashTemporaryPermissionTrackerFactory",
          BrowserContextDependencyManager::GetInstance()) {}

FlashTemporaryPermissionTrackerFactory::
    ~FlashTemporaryPermissionTrackerFactory() {}

scoped_refptr<RefcountedKeyedService>
FlashTemporaryPermissionTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new FlashTemporaryPermissionTracker(
      Profile::FromBrowserContext(context));
}

content::BrowserContext*
FlashTemporaryPermissionTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
