// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/shortcut_manager_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/apps/shortcut_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
AppShortcutManager* AppShortcutManagerFactory::GetForProfile(Profile* profile) {
  return static_cast<AppShortcutManager*>(
      GetInstance()->GetServiceForBrowserContext(profile,
                                                 false /* don't create */));
}

AppShortcutManagerFactory* AppShortcutManagerFactory::GetInstance() {
  return base::Singleton<AppShortcutManagerFactory>::get();
}

AppShortcutManagerFactory::AppShortcutManagerFactory()
    : BrowserContextKeyedServiceFactory(
        "AppShortcutManager",
        BrowserContextDependencyManager::GetInstance()) {
}

AppShortcutManagerFactory::~AppShortcutManagerFactory() {
}

KeyedService* AppShortcutManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new AppShortcutManager(static_cast<Profile*>(profile));
}

bool AppShortcutManagerFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

bool AppShortcutManagerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
