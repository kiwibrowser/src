// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_PROXY_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_PROXY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/ntp_snippets/contextual/contextual_content_suggestions_service.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_result.h"

class GURL;

namespace contextual_suggestions {

// Class providing access to the Contextual Content Suggestions Service and
// caching the list of current suggestions shown in a tab. It is owned by a UI
// object. There could be multiple instances of serivce proxy. Either can be
// torn down with a part of UI that owns it, which doesn't affect other proxies.
class ContextualContentSuggestionsServiceProxy {
 public:
  ContextualContentSuggestionsServiceProxy(
      ContextualContentSuggestionsService* service,
      std::unique_ptr<ContextualSuggestionsReporter> reporter);
  ~ContextualContentSuggestionsServiceProxy();

  // Fetches contextual suggestions for a given |url|.
  void FetchContextualSuggestions(const GURL& url,
                                  FetchClustersCallback callback);

  // Fetches an image for a contextual suggestion with specified
  // |suggestion_id|.
  void FetchContextualSuggestionImage(
      const std::string& suggestion_id,
      ntp_snippets::ImageFetchedCallback callback);

  // Fetches a favicon for a contextual suggestion with specified
  // |suggestion_id|.
  void FetchContextualSuggestionFavicon(
      const std::string& suggestion_id,
      ntp_snippets::ImageFetchedCallback callback);

  // Clears the state of the proxy.
  void ClearState();

  // Reports user interface event to the service.
  void ReportEvent(ukm::SourceId,
                   const std::string& url,
                   ContextualSuggestionsEvent event);

  // Ensures that all metrics are properly flushed.
  void FlushMetrics();

 private:
  void FetchImageImpl(const GURL& image_url,
                      const std::string& image_id,
                      ntp_snippets::ImageFetchedCallback callback);

  void CacheSuggestions(FetchClustersCallback callback,
                        ContextualSuggestionsResult result);
  // Pointer to the service.
  ContextualContentSuggestionsService* service_;
  // Cache of contextual suggestions.
  std::map<std::string, ContextualSuggestion> suggestions_;

  // Sink for reporting metrics for this proxy.
  std::unique_ptr<contextual_suggestions::ContextualSuggestionsReporter>
      reporter_;

  // The most recent SourceId in use by metrics_reporter_, or
  // ukm::kInvalidSourceId.
  ukm::SourceId last_ukm_source_id_;

  // Weak pointer factory to cancel pending callbacks.
  base::WeakPtrFactory<ContextualContentSuggestionsServiceProxy>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContextualContentSuggestionsServiceProxy);
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_CONTENT_SUGGESTIONS_SERVICE_PROXY_H_
