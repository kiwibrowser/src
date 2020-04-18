// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_FETCHER_IMPL_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_FETCHER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "components/ntp_snippets/contextual/contextual_suggestion.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_fetch.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_fetcher.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

using contextual_suggestions::Cluster;
using contextual_suggestions::ContextualSuggestionsFetch;
using contextual_suggestions::ContextualSuggestionsResult;

namespace contextual_suggestions {

class ContextualSuggestionsFetcherImpl : public ContextualSuggestionsFetcher {
 public:
  ContextualSuggestionsFetcherImpl(
      const scoped_refptr<network::SharedURLLoaderFactory>& loader_factory,
      const std::string& application_language_code);
  ~ContextualSuggestionsFetcherImpl() override;

  // ContextualSuggestionsFetcher implementation.
  void FetchContextualSuggestionsClusters(
      const GURL& url,
      FetchClustersCallback callback,
      ReportFetchMetricsCallback metrics_callback) override;

 private:
  void FetchFinished(ContextualSuggestionsFetch* fetch,
                     FetchClustersCallback callback,
                     ContextualSuggestionsResult result);

  const scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  /// BCP47 formatted language code to use.
  const std::string bcp_language_code_;

  // Stores requests that are inflight.
  base::flat_set<std::unique_ptr<ContextualSuggestionsFetch>,
                 base::UniquePtrComparator>
      pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsFetcherImpl);
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_FETCHER_IMPL_H_
