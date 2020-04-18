// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bookmarks/enhanced_bookmark_key_service_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/ui/bookmarks/enhanced_bookmark_key_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
EnhancedBookmarkKeyServiceFactory*
    EnhancedBookmarkKeyServiceFactory::GetInstance() {
  return base::Singleton<EnhancedBookmarkKeyServiceFactory>::get();
}

EnhancedBookmarkKeyServiceFactory::EnhancedBookmarkKeyServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "EnhancedBookmarkKeyService",
        BrowserContextDependencyManager::GetInstance()) {
}

EnhancedBookmarkKeyServiceFactory::~EnhancedBookmarkKeyServiceFactory() {
}

KeyedService* EnhancedBookmarkKeyServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new EnhancedBookmarkKeyService(context);
}

content::BrowserContext*
    EnhancedBookmarkKeyServiceFactory::GetBrowserContextToUse(
        content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool EnhancedBookmarkKeyServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool EnhancedBookmarkKeyServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
