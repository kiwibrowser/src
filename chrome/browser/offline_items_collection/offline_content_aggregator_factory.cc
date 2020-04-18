// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "content/public/browser/browser_context.h"

// static
OfflineContentAggregatorFactory*
OfflineContentAggregatorFactory::GetInstance() {
  return base::Singleton<OfflineContentAggregatorFactory>::get();
}

// static
offline_items_collection::OfflineContentAggregator*
OfflineContentAggregatorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(!context->IsOffTheRecord());
  return static_cast<offline_items_collection::OfflineContentAggregator*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

OfflineContentAggregatorFactory::OfflineContentAggregatorFactory()
    : BrowserContextKeyedServiceFactory(
          "offline_items_collection::OfflineContentAggregator",
          BrowserContextDependencyManager::GetInstance()) {}

OfflineContentAggregatorFactory::~OfflineContentAggregatorFactory() = default;

KeyedService* OfflineContentAggregatorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new offline_items_collection::OfflineContentAggregator();
}

content::BrowserContext*
OfflineContentAggregatorFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
