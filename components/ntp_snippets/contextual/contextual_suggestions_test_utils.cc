// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_test_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace contextual_suggestions {

MockClustersCallback::MockClustersCallback() = default;
MockClustersCallback::MockClustersCallback(const MockClustersCallback& other) =
    default;
MockClustersCallback::~MockClustersCallback() = default;

void MockClustersCallback::Done(ContextualSuggestionsResult result) {
  EXPECT_FALSE(has_run);
  has_run = true;
  response_peek_text = result.peek_text;
  response_clusters = std::move(result.clusters);
  peek_conditions = result.peek_conditions;
}

FetchClustersCallback MockClustersCallback::ToOnceCallback() {
  return base::BindOnce(&MockClustersCallback::Done, base::Unretained(this));
}

MockMetricsCallback::MockMetricsCallback() = default;
MockMetricsCallback::~MockMetricsCallback() = default;

void MockMetricsCallback::Report(ContextualSuggestionsEvent event) {
  events.push_back(event);
}

void ExpectSuggestionsMatch(const ContextualSuggestion& actual,
                            const ContextualSuggestion& expected) {
  EXPECT_EQ(actual.id, expected.id);
  EXPECT_EQ(actual.title, expected.title);
  EXPECT_EQ(actual.url, expected.url);
  EXPECT_EQ(actual.snippet, expected.snippet);
  EXPECT_EQ(actual.publisher_name, expected.publisher_name);
  EXPECT_EQ(actual.image_id, expected.image_id);
  EXPECT_EQ(actual.favicon_image_id, expected.favicon_image_id);
}

void ExpectClustersMatch(const Cluster& actual, const Cluster& expected) {
  EXPECT_EQ(actual.title, expected.title);
  ASSERT_EQ(actual.suggestions.size(), expected.suggestions.size());
  for (size_t i = 0; i < actual.suggestions.size(); i++) {
    ExpectSuggestionsMatch(std::move(actual.suggestions[i]),
                           std::move(expected.suggestions[i]));
  }
}

void ExpectPeekConditionsMatch(const PeekConditions& actual,
                               const PeekConditions& expected) {
  EXPECT_EQ(actual.confidence, expected.confidence);
  EXPECT_EQ(actual.page_scroll_percentage, expected.page_scroll_percentage);
  EXPECT_EQ(actual.minimum_seconds_on_page, expected.minimum_seconds_on_page);
  EXPECT_EQ(actual.maximum_number_of_peeks, expected.maximum_number_of_peeks);
}

void ExpectResponsesMatch(const MockClustersCallback& callback,
                          const ContextualSuggestionsResult& expected_result) {
  EXPECT_EQ(callback.response_peek_text, expected_result.peek_text);
  ExpectPeekConditionsMatch(callback.peek_conditions,
                            expected_result.peek_conditions);
  ASSERT_EQ(callback.response_clusters.size(), expected_result.clusters.size());
  for (size_t i = 0; i < callback.response_clusters.size(); i++) {
    ExpectClustersMatch(std::move(callback.response_clusters[i]),
                        std::move(expected_result.clusters[i]));
  }
}

}  // namespace contextual_suggestions
