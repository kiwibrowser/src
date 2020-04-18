// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/nqe/ui_network_quality_estimator_service_factory.h"

#include "chrome/browser/net/nqe/ui_network_quality_estimator_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

// static
UINetworkQualityEstimatorService*
UINetworkQualityEstimatorServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<UINetworkQualityEstimatorService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
UINetworkQualityEstimatorServiceFactory*
UINetworkQualityEstimatorServiceFactory::GetInstance() {
  return base::Singleton<UINetworkQualityEstimatorServiceFactory>::get();
}

UINetworkQualityEstimatorServiceFactory::
    UINetworkQualityEstimatorServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "UINetworkQualityEstimatorService",
          BrowserContextDependencyManager::GetInstance()) {}

UINetworkQualityEstimatorServiceFactory::
    ~UINetworkQualityEstimatorServiceFactory() {}

bool UINetworkQualityEstimatorServiceFactory::
    ServiceIsCreatedWithBrowserContext() const {
  // Initialize the UI network quality estimator service so it can
  // read/write the prefs.
  return true;
}

KeyedService* UINetworkQualityEstimatorServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new UINetworkQualityEstimatorService(
      Profile::FromBrowserContext(context));
}
