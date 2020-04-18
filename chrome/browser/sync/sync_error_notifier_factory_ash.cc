// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sync_error_notifier_factory_ash.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/sync_error_notifier_ash.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

SyncErrorNotifierFactory::SyncErrorNotifierFactory()
    : BrowserContextKeyedServiceFactory(
        "SyncErrorNotifier",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileSyncServiceFactory::GetInstance());
}

SyncErrorNotifierFactory::~SyncErrorNotifierFactory() {}

// static
SyncErrorNotifier* SyncErrorNotifierFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SyncErrorNotifier*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SyncErrorNotifierFactory* SyncErrorNotifierFactory::GetInstance() {
  return base::Singleton<SyncErrorNotifierFactory>::get();
}

KeyedService* SyncErrorNotifierFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  browser_sync::ProfileSyncService* profile_sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile);

  if (!profile_sync_service)
    return nullptr;

  syncer::SyncErrorController* sync_error_controller =
      profile_sync_service->sync_error_controller();
  if (!sync_error_controller)
    return nullptr;

  return new SyncErrorNotifier(sync_error_controller, profile);
}
