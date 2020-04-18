// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/optional.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/ntp_snippets/callbacks.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_fetcher.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace ntp_snippets {
class CachedImageFetcher;
class RemoteSuggestionsDatabase;
}  // namespace ntp_snippets

namespace contextual_suggestions {

class ContextualContentSuggestionsServiceProxy;

// Retrieves contextual suggestions for a given URL and fetches images
// for contextual suggestion, using caching.
class ContextualContentSuggestionsService : public KeyedService {
 public:
  ContextualContentSuggestionsService(
      std::unique_ptr<ContextualSuggestionsFetcher>
          contextual_suggestions_fetcher,
      std::unique_ptr<ntp_snippets::CachedImageFetcher> image_fetcher,
      std::unique_ptr<ntp_snippets::RemoteSuggestionsDatabase>
          contextual_suggestions_database,
      std::unique_ptr<
          contextual_suggestions::ContextualSuggestionsReporterProvider>
          reporter_provider);
  ~ContextualContentSuggestionsService() override;

  // Asynchronously fetches contextual suggestions for the given URL.
  virtual void FetchContextualSuggestionClusters(
      const GURL& url,
      FetchClustersCallback callback,
      ReportFetchMetricsCallback metrics_callback);

  // Fetches an image pointed to by |url| and internally caches it using
  // |suggestion_id|. Asynchronous if cache or network is queried.
  virtual void FetchContextualSuggestionImage(
      const ntp_snippets::ContentSuggestion::ID& suggestion_id,
      const GURL& url,
      ntp_snippets::ImageFetchedCallback callback);

  void FetchDone(FetchClustersCallback callback,
                 ReportFetchMetricsCallback metrics_callback,
                 ContextualSuggestionsResult result);

  std::unique_ptr<ContextualContentSuggestionsServiceProxy> CreateProxy();

 private:
  // Cache for images of contextual suggestions, needed by CachedImageFetcher.
  std::unique_ptr<ntp_snippets::RemoteSuggestionsDatabase>
      contextual_suggestions_database_;

  // Performs actual network request to fetch contextual suggestions.
  std::unique_ptr<ContextualSuggestionsFetcher> contextual_suggestions_fetcher_;

  std::unique_ptr<ntp_snippets::CachedImageFetcher> image_fetcher_;

  std::unique_ptr<contextual_suggestions::ContextualSuggestionsReporterProvider>
      reporter_provider_;

  // Look up by ContentSuggestion::ID::id_within_category() aka std::string to
  // get image URL.
  std::map<std::string, GURL> image_url_by_id_;

  DISALLOW_COPY_AND_ASSIGN(ContextualContentSuggestionsService);
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_H_
