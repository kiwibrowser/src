// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/feed/feed_host_service_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/suggestions/image_decoder_impl.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/common/channel_info.h"
#include "components/feed/core/feed_host_service.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "google_apis/google_api_keys.h"
#include "net/url_request/url_request_context_getter.h"

namespace feed {

namespace {
const char kFeedFolder[] = "feed";
}  // namespace

// static
FeedHostService* FeedHostServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<FeedHostService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
FeedHostServiceFactory* FeedHostServiceFactory::GetInstance() {
  return base::Singleton<FeedHostServiceFactory>::get();
}

FeedHostServiceFactory::FeedHostServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "FeedHostService",
          BrowserContextDependencyManager::GetInstance()) {}

FeedHostServiceFactory::~FeedHostServiceFactory() = default;

KeyedService* FeedHostServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  content::StoragePartition* storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(context);

  identity::IdentityManager* identity_manager =
      IdentityManagerFactory::GetForProfile(profile);
  std::string api_key;
  if (google_apis::IsGoogleChromeAPIKeyUsed()) {
    bool is_stable_channel =
        chrome::GetChannel() == version_info::Channel::STABLE;
    api_key = is_stable_channel ? google_apis::GetAPIKey()
                                : google_apis::GetNonStableAPIKey();
  }
  auto networking_host = std::make_unique<FeedNetworkingHost>(
      identity_manager, api_key,
      storage_partition->GetURLLoaderFactoryForBrowserProcess());

  scoped_refptr<net::URLRequestContextGetter> request_context =
      profile->GetRequestContext();

  auto image_fetcher = std::make_unique<image_fetcher::ImageFetcherImpl>(
      std::make_unique<suggestions::ImageDecoderImpl>(), request_context.get());

  base::FilePath feed_dir(profile->GetPath().Append(kFeedFolder));
  auto image_database = std::make_unique<FeedImageDatabase>(feed_dir);

  auto image_manager = std::make_unique<FeedImageManager>(
      std::move(image_fetcher), std::move(image_database));

  return new FeedHostService(std::move(image_manager),
                             std::move(networking_host));
}

content::BrowserContext* FeedHostServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context->IsOffTheRecord() ? nullptr : context;
}

}  // namespace feed
