// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_TEST_UTILS_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_TEST_UTILS_H_

#include "base/bind.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_result.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace contextual_suggestions {

// ClustersCallback implementation for testing purposes that expects to be
// called only once, and remembers the results of that call in public members.
class MockClustersCallback {
 public:
  MockClustersCallback();
  MockClustersCallback(const MockClustersCallback&);
  ~MockClustersCallback();

  void Done(ContextualSuggestionsResult result);
  FetchClustersCallback ToOnceCallback();

  bool has_run = false;
  std::string response_peek_text;
  std::vector<Cluster> response_clusters;
  PeekConditions peek_conditions;
};

class MockMetricsCallback {
 public:
  MockMetricsCallback();
  ~MockMetricsCallback();

  void Report(ContextualSuggestionsEvent event);

  std::vector<ContextualSuggestionsEvent> events;
};

void ExpectSuggestionsMatch(const ContextualSuggestion& actual,
                            const ContextualSuggestion& expected);
void ExpectClustersMatch(const Cluster& actual, const Cluster& expected);
void ExpectPeekConditionsMatch(const PeekConditions& actual,
                               const PeekConditions& expected);

// Expect that the individual data members saved in |callback| match the
// corresponding data in |expected_result|.
void ExpectResponsesMatch(const MockClustersCallback& callback,
                          const ContextualSuggestionsResult& expected_result);

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_TEST_UTILS_H_
