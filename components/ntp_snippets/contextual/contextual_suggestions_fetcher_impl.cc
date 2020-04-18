// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_fetcher_impl.h"

#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace contextual_suggestions {

ContextualSuggestionsFetcherImpl::ContextualSuggestionsFetcherImpl(
    const scoped_refptr<network::SharedURLLoaderFactory>& loader_factory,
    const std::string& application_language_code)
    : loader_factory_(loader_factory),
      bcp_language_code_(application_language_code) {}

ContextualSuggestionsFetcherImpl::~ContextualSuggestionsFetcherImpl() = default;

void ContextualSuggestionsFetcherImpl::FetchContextualSuggestionsClusters(
    const GURL& url,
    FetchClustersCallback callback,
    ReportFetchMetricsCallback metrics_callback) {
  auto fetch =
      std::make_unique<ContextualSuggestionsFetch>(url, bcp_language_code_);
  ContextualSuggestionsFetch* fetch_unowned = fetch.get();
  pending_requests_.emplace(std::move(fetch));

  FetchClustersCallback internal_callback = base::BindOnce(
      &ContextualSuggestionsFetcherImpl::FetchFinished, base::Unretained(this),
      fetch_unowned, std::move(callback));
  fetch_unowned->Start(std::move(internal_callback),
                       std::move(metrics_callback), loader_factory_);
}

void ContextualSuggestionsFetcherImpl::FetchFinished(
    ContextualSuggestionsFetch* fetch,
    FetchClustersCallback callback,
    ContextualSuggestionsResult result) {
  auto fetch_iterator = pending_requests_.find(fetch);
  CHECK(fetch_iterator != pending_requests_.end());
  pending_requests_.erase(fetch_iterator);

  std::move(callback).Run(std::move(result));
}

}  // namespace contextual_suggestions
