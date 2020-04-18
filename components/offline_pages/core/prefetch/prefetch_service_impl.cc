// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_service_impl.h"

#include <utility>

#include "base/bind.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/prefetch/offline_metrics_collector.h"
#include "components/offline_pages/core/prefetch/prefetch_background_task_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_configuration.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_downloader.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_importer.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store.h"
#include "components/offline_pages/core/prefetch/suggested_articles_observer.h"
#include "components/offline_pages/core/prefetch/thumbnail_fetcher.h"

namespace offline_pages {

PrefetchServiceImpl::PrefetchServiceImpl(
    std::unique_ptr<OfflineMetricsCollector> offline_metrics_collector,
    std::unique_ptr<PrefetchDispatcher> dispatcher,
    std::unique_ptr<PrefetchGCMHandler> gcm_handler,
    std::unique_ptr<PrefetchNetworkRequestFactory> network_request_factory,
    OfflinePageModel* offline_page_model,
    std::unique_ptr<PrefetchStore> prefetch_store,
    std::unique_ptr<SuggestedArticlesObserver> suggested_articles_observer,
    std::unique_ptr<PrefetchDownloader> prefetch_downloader,
    std::unique_ptr<PrefetchImporter> prefetch_importer,
    std::unique_ptr<PrefetchBackgroundTaskHandler>
        prefetch_background_task_handler,
    std::unique_ptr<PrefetchConfiguration> prefetch_configuration,
    std::unique_ptr<ThumbnailFetcher> thumbnail_fetcher)
    : offline_metrics_collector_(std::move(offline_metrics_collector)),
      prefetch_dispatcher_(std::move(dispatcher)),
      prefetch_gcm_handler_(std::move(gcm_handler)),
      network_request_factory_(std::move(network_request_factory)),
      offline_page_model_(offline_page_model),
      prefetch_store_(std::move(prefetch_store)),
      suggested_articles_observer_(std::move(suggested_articles_observer)),
      prefetch_downloader_(std::move(prefetch_downloader)),
      prefetch_importer_(std::move(prefetch_importer)),
      prefetch_background_task_handler_(
          std::move(prefetch_background_task_handler)),
      prefetch_configuration_(std::move(prefetch_configuration)),
      thumbnail_fetcher_(std::move(thumbnail_fetcher)) {
  prefetch_dispatcher_->SetService(this);
  prefetch_downloader_->SetPrefetchService(this);
  prefetch_gcm_handler_->SetService(this);
  suggested_articles_observer_->SetPrefetchService(this);
}

PrefetchServiceImpl::~PrefetchServiceImpl() {
  // The dispatcher needs to be disposed first because it may need to
  // communicate with other members owned by the service at destruction time.
  prefetch_dispatcher_.reset();
}

void PrefetchServiceImpl::SetContentSuggestionsService(
    ntp_snippets::ContentSuggestionsService* content_suggestions) {
  suggested_articles_observer_->SetContentSuggestionsServiceAndObserve(
      content_suggestions);
  thumbnail_fetcher_->SetContentSuggestionsService(content_suggestions);
}

OfflineMetricsCollector* PrefetchServiceImpl::GetOfflineMetricsCollector() {
  return offline_metrics_collector_.get();
}

PrefetchDispatcher* PrefetchServiceImpl::GetPrefetchDispatcher() {
  return prefetch_dispatcher_.get();
}

PrefetchGCMHandler* PrefetchServiceImpl::GetPrefetchGCMHandler() {
  return prefetch_gcm_handler_.get();
}

PrefetchNetworkRequestFactory*
PrefetchServiceImpl::GetPrefetchNetworkRequestFactory() {
  return network_request_factory_.get();
}

OfflinePageModel* PrefetchServiceImpl::GetOfflinePageModel() {
  return offline_page_model_;
}

PrefetchStore* PrefetchServiceImpl::GetPrefetchStore() {
  return prefetch_store_.get();
}

SuggestedArticlesObserver* PrefetchServiceImpl::GetSuggestedArticlesObserver() {
  return suggested_articles_observer_.get();
}

OfflineEventLogger* PrefetchServiceImpl::GetLogger() {
  return &logger_;
}

PrefetchDownloader* PrefetchServiceImpl::GetPrefetchDownloader() {
  return prefetch_downloader_.get();
}

PrefetchImporter* PrefetchServiceImpl::GetPrefetchImporter() {
  return prefetch_importer_.get();
}

PrefetchBackgroundTaskHandler*
PrefetchServiceImpl::GetPrefetchBackgroundTaskHandler() {
  return prefetch_background_task_handler_.get();
}

PrefetchConfiguration* PrefetchServiceImpl::GetPrefetchConfiguration() {
  return prefetch_configuration_.get();
}

ThumbnailFetcher* PrefetchServiceImpl::GetThumbnailFetcher() {
  return thumbnail_fetcher_.get();
}

void PrefetchServiceImpl::Shutdown() {
  suggested_articles_observer_.reset();
  prefetch_downloader_.reset();
}

}  // namespace offline_pages
