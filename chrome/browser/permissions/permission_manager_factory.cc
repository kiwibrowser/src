// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/permissions/permission_manager_factory.h"

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
PermissionManager*
PermissionManagerFactory::GetForProfile(Profile* profile) {
  return static_cast<PermissionManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
PermissionManagerFactory* PermissionManagerFactory::GetInstance() {
  return base::Singleton<PermissionManagerFactory>::get();
}

PermissionManagerFactory::PermissionManagerFactory()
    : BrowserContextKeyedServiceFactory(
        "PermissionManagerFactory",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

PermissionManagerFactory::~PermissionManagerFactory() {
}

KeyedService* PermissionManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PermissionManager(Profile::FromBrowserContext(context));
}

content::BrowserContext*
PermissionManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
