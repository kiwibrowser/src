// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_package_syncable_service_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs_factory.h"
#include "chrome/browser/ui/app_list/arc/arc_package_syncable_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

namespace arc {

// static
ArcPackageSyncableService*
ArcPackageSyncableServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ArcPackageSyncableService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ArcPackageSyncableServiceFactory*
ArcPackageSyncableServiceFactory::GetInstance() {
  return base::Singleton<ArcPackageSyncableServiceFactory>::get();
}

ArcPackageSyncableServiceFactory::ArcPackageSyncableServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ArcPackageSyncableService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ArcAppListPrefsFactory::GetInstance());
}

ArcPackageSyncableServiceFactory::~ArcPackageSyncableServiceFactory() {}

KeyedService* ArcPackageSyncableServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  DCHECK(profile);

  return ArcPackageSyncableService::Create(profile,
                                           ArcAppListPrefs::Get(profile));
}

content::BrowserContext*
ArcPackageSyncableServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // This matches the logic in ExtensionSyncServiceFactory, which uses the
  // orginal browser context.
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace arc
