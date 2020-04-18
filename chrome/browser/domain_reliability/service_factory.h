// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOMAIN_RELIABILITY_SERVICE_FACTORY_H_
#define CHROME_BROWSER_DOMAIN_RELIABILITY_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace domain_reliability {

class DomainReliabilityService;

// Creates DomainReliabilityServices for BrowserContexts. Initializes them with
// the hardcoded upload reporter string "chrome".
class DomainReliabilityServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static DomainReliabilityService* GetForBrowserContext(
      content::BrowserContext* context);

  static DomainReliabilityServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DomainReliabilityServiceFactory>;

  DomainReliabilityServiceFactory();
  ~DomainReliabilityServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  bool ShouldCreateService() const;

  DISALLOW_COPY_AND_ASSIGN(DomainReliabilityServiceFactory);
};

}  // namespace domain_reliability

#endif  // CHROME_BROWSER_DOMAIN_RELIABILITY_SERVICE_FACTORY_H_
