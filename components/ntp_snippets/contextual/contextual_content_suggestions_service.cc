// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_content_suggestions_service.h"

#include <iterator>
#include <memory>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_result.h"
#include "components/ntp_snippets/remote/cached_image_fetcher.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/ntp_snippets/remote/remote_suggestions_provider_impl.h"
#include "contextual_content_suggestions_service_proxy.h"
#include "ui/gfx/image/image.h"

namespace contextual_suggestions {

using ntp_snippets::ContentSuggestion;
using ntp_snippets::ImageDataFetchedCallback;
using ntp_snippets::ImageFetchedCallback;
using ntp_snippets::CachedImageFetcher;
using ntp_snippets::RemoteSuggestionsDatabase;

namespace {
bool IsEligibleURL(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS() && !url.HostIsIPAddress();
}

static constexpr float kMinimumConfidence = 0.75;

}  // namespace

ContextualContentSuggestionsService::ContextualContentSuggestionsService(
    std::unique_ptr<ContextualSuggestionsFetcher>
        contextual_suggestions_fetcher,
    std::unique_ptr<CachedImageFetcher> image_fetcher,
    std::unique_ptr<RemoteSuggestionsDatabase> contextual_suggestions_database,
    std::unique_ptr<ContextualSuggestionsReporterProvider> reporter_provider)
    : contextual_suggestions_database_(
          std::move(contextual_suggestions_database)),
      contextual_suggestions_fetcher_(
          std::move(contextual_suggestions_fetcher)),
      image_fetcher_(std::move(image_fetcher)),
      reporter_provider_(std::move(reporter_provider)) {}

ContextualContentSuggestionsService::~ContextualContentSuggestionsService() =
    default;

void ContextualContentSuggestionsService::FetchContextualSuggestionClusters(
    const GURL& url,
    FetchClustersCallback callback,
    ReportFetchMetricsCallback metrics_callback) {
  // TODO(pnoland): Also check that the url is safe.
  if (IsEligibleURL(url)) {
    FetchClustersCallback internal_callback = base::BindOnce(
        &ContextualContentSuggestionsService::FetchDone, base::Unretained(this),
        std::move(callback), metrics_callback);
    contextual_suggestions_fetcher_->FetchContextualSuggestionsClusters(
        url, std::move(internal_callback), metrics_callback);
  } else {
    std::move(callback).Run(
        ContextualSuggestionsResult("", {}, PeekConditions()));
  }
}

void ContextualContentSuggestionsService::FetchContextualSuggestionImage(
    const ContentSuggestion::ID& suggestion_id,
    const GURL& image_url,
    ImageFetchedCallback callback) {
  image_fetcher_->FetchSuggestionImage(suggestion_id, image_url,
                                       ImageDataFetchedCallback(),
                                       std::move(callback));
}

void ContextualContentSuggestionsService::FetchDone(
    FetchClustersCallback callback,
    ReportFetchMetricsCallback metrics_callback,
    ContextualSuggestionsResult result) {
  if (result.peek_conditions.confidence < kMinimumConfidence) {
    metrics_callback.Run(contextual_suggestions::FETCH_BELOW_THRESHOLD);
    std::move(callback).Run(
        ContextualSuggestionsResult("", {}, PeekConditions()));
    return;
  }

  std::move(callback).Run(result);
}

std::unique_ptr<
    contextual_suggestions::ContextualContentSuggestionsServiceProxy>
ContextualContentSuggestionsService::CreateProxy() {
  return std::make_unique<
      contextual_suggestions::ContextualContentSuggestionsServiceProxy>(
      this, reporter_provider_->CreateReporter());
}

}  // namespace contextual_suggestions
