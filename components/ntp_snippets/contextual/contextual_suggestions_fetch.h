// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_FETCH_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_FETCH_H_

#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_result.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace contextual_suggestions {

// A fetch of contextual suggestions. This encapsulates the request-response
// lifecycle. It is also responsible for building and serializing the request
// body protos and parsing the response body protos.
class ContextualSuggestionsFetch {
 public:
  ContextualSuggestionsFetch(const GURL& url, const std::string& bcp_language);
  ~ContextualSuggestionsFetch();

  // Get the url used to fetch suggestions.
  static const std::string GetFetchEndpoint();

  // Start fetching suggestions using |loader_factory| to construct a
  // URLLoader, and calling |callback| when finished.
  void Start(
      FetchClustersCallback callback,
      ReportFetchMetricsCallback metrics_callback,
      const scoped_refptr<network::SharedURLLoaderFactory>& loader_factory);

 private:
  std::unique_ptr<network::SimpleURLLoader> MakeURLLoader() const;
  net::HttpRequestHeaders MakeHeaders() const;
  void OnURLLoaderComplete(ReportFetchMetricsCallback metrics_callback,
                           std::unique_ptr<std::string> result);
  void ReportFetchMetrics(int32_t error_code,
                          int32_t response_code,
                          size_t clusters_size,
                          ReportFetchMetricsCallback metrics_callback);

  // The url for which we're fetching suggestions.
  const GURL url_;

  // Identifier for the spoken language in BCP47 format.
  const std::string bcp_language_code_;

  // The loader for downloading the suggestions. Only non-null if a fetch is
  // currently ongoing.
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  // The callback to notify when results are available.
  FetchClustersCallback request_completed_callback_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsFetch);
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_FETCH_H_
