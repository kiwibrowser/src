// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/background/ntp_background_service_factory.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/optional.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/background/ntp_background_service.h"
#include "chrome/common/chrome_features.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

// static
NtpBackgroundService* NtpBackgroundServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<NtpBackgroundService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
NtpBackgroundServiceFactory* NtpBackgroundServiceFactory::GetInstance() {
  return base::Singleton<NtpBackgroundServiceFactory>::get();
}

NtpBackgroundServiceFactory::NtpBackgroundServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "NtpBackgroundService",
          BrowserContextDependencyManager::GetInstance()) {}

NtpBackgroundServiceFactory::~NtpBackgroundServiceFactory() = default;

KeyedService* NtpBackgroundServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(features::kNtpBackgrounds)) {
    return nullptr;
  }

  std::string collections_api_url = base::GetFieldTrialParamValueByFeature(
      features::kNtpBackgrounds, "background-collections-api-url");
  std::string collection_images_api_url =
      base::GetFieldTrialParamValueByFeature(
          features::kNtpBackgrounds, "background-collection-images-api-url");
  std::string image_options = base::GetFieldTrialParamValueByFeature(
      features::kNtpBackgrounds, "background-collections-image-options");
  base::Optional<GURL> collection_api_url_override;
  base::Optional<GURL> collection_images_api_url_override;
  base::Optional<std::string> image_options_override;
  if (!collections_api_url.empty()) {
    collection_api_url_override = GURL(collections_api_url);
  }
  if (!collection_images_api_url.empty()) {
    collection_images_api_url_override = GURL(collection_images_api_url);
  }
  if (!image_options.empty()) {
    image_options_override = image_options;
  }

  auto url_loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(context)
          ->GetURLLoaderFactoryForBrowserProcess();
  return new NtpBackgroundService(
      url_loader_factory, collection_api_url_override,
      collection_images_api_url_override, image_options_override);
}
