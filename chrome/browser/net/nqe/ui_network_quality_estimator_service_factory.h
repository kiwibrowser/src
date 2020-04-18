// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_FACTORY_H_
#define CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace content {
class BrowserContext;
}

class UINetworkQualityEstimatorService;

class UINetworkQualityEstimatorServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static UINetworkQualityEstimatorService* GetForProfile(Profile* profile);

  static UINetworkQualityEstimatorServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      UINetworkQualityEstimatorServiceFactory>;

  UINetworkQualityEstimatorServiceFactory();
  ~UINetworkQualityEstimatorServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_FACTORY_H_
