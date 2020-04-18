// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_ITEMS_COLLECTION_OFFLINE_CONTENT_AGGREGATOR_FACTORY_H_
#define CHROME_BROWSER_OFFLINE_ITEMS_COLLECTION_OFFLINE_CONTENT_AGGREGATOR_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace offline_items_collection {
class OfflineContentAggregator;
}  // namespace offline_items_collection

// This class is the main access point for an OfflineContentAggregator.  It is
// responsible for building the OfflineContentAggregator and associating it with
// a particular content::BrowserContext.
class OfflineContentAggregatorFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns a singleton instance of an OfflineContentAggregatorFactory.
  static OfflineContentAggregatorFactory* GetInstance();

  // Returns the OfflineContentAggregator associated with |context| or creates
  // and associates one if it doesn't exist.
  static offline_items_collection::OfflineContentAggregator*
  GetForBrowserContext(content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<OfflineContentAggregatorFactory>;

  OfflineContentAggregatorFactory();
  ~OfflineContentAggregatorFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(OfflineContentAggregatorFactory);
};

#endif  // CHROME_BROWSER_OFFLINE_ITEMS_COLLECTION_OFFLINE_CONTENT_AGGREGATOR_FACTORY_H_
