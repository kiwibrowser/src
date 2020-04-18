// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/drive/drive_notification_manager_factory.h"

#include "base/logging.h"
#include "chrome/browser/invalidation/profile_invalidation_provider_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/drive/drive_notification_manager.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace drive {

// static
DriveNotificationManager*
DriveNotificationManagerFactory::FindForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<DriveNotificationManager*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

// static
DriveNotificationManager*
DriveNotificationManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  if (!browser_sync::ProfileSyncService::IsSyncAllowedByFlag())
    return NULL;
  if (!invalidation::ProfileInvalidationProviderFactory::GetForProfile(
          Profile::FromBrowserContext(context))) {
    // Do not create a DriveNotificationManager for |context|s that do not
    // support invalidation.
    return NULL;
  }

  return static_cast<DriveNotificationManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
DriveNotificationManagerFactory*
DriveNotificationManagerFactory::GetInstance() {
  return base::Singleton<DriveNotificationManagerFactory>::get();
}

DriveNotificationManagerFactory::DriveNotificationManagerFactory()
    : BrowserContextKeyedServiceFactory(
        "DriveNotificationManager",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileSyncServiceFactory::GetInstance());
  DependsOn(invalidation::ProfileInvalidationProviderFactory::GetInstance());
}

DriveNotificationManagerFactory::~DriveNotificationManagerFactory() {}

KeyedService* DriveNotificationManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  invalidation::ProfileInvalidationProvider* invalidation_provider =
      invalidation::ProfileInvalidationProviderFactory::GetForProfile(
          Profile::FromBrowserContext(context));
  DCHECK(invalidation_provider);
  DCHECK(invalidation_provider->GetInvalidationService());
  return new DriveNotificationManager(
      invalidation_provider->GetInvalidationService());
}

}  // namespace drive
