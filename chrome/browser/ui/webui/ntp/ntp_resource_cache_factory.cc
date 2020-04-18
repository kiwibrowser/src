// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/ntp_resource_cache_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/webui/ntp/ntp_resource_cache.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
NTPResourceCache* NTPResourceCacheFactory::GetForProfile(Profile* profile) {
  return static_cast<NTPResourceCache*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
NTPResourceCacheFactory* NTPResourceCacheFactory::GetInstance() {
  return base::Singleton<NTPResourceCacheFactory>::get();
}

NTPResourceCacheFactory::NTPResourceCacheFactory()
    : BrowserContextKeyedServiceFactory(
        "NTPResourceCache",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SigninManagerFactory::GetInstance());
  DependsOn(ThemeServiceFactory::GetInstance());
}

NTPResourceCacheFactory::~NTPResourceCacheFactory() {}

KeyedService* NTPResourceCacheFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new NTPResourceCache(static_cast<Profile*>(profile));
}

content::BrowserContext* NTPResourceCacheFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
