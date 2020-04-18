// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_vpn_provider_manager_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs_factory.h"
#include "chrome/browser/ui/app_list/arc/arc_vpn_provider_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace app_list {

// static
ArcVpnProviderManager* ArcVpnProviderManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ArcVpnProviderManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ArcVpnProviderManagerFactory* ArcVpnProviderManagerFactory::GetInstance() {
  return base::Singleton<ArcVpnProviderManagerFactory>::get();
}

ArcVpnProviderManagerFactory::ArcVpnProviderManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "ArcVpnProviderManager",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ArcAppListPrefsFactory::GetInstance());
}

ArcVpnProviderManagerFactory::~ArcVpnProviderManagerFactory() {}

KeyedService* ArcVpnProviderManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return ArcVpnProviderManager::Create(context);
}

content::BrowserContext* ArcVpnProviderManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // This matches the logic in ExtensionSyncServiceFactory, which uses the
  // orginal browser context.
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace app_list
