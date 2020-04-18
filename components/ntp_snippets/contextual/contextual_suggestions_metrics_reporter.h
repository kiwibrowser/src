// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_METRICS_REPORTER_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_METRICS_REPORTER_H_

#include <memory>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace contextual_suggestions {

class ContextualSuggestionsUkmEntry;

FORWARD_DECLARE_TEST(ContextualSuggestionsMetricsReporterTest, BaseTest);

// Reporter specialized for UKM/UMA metrics reporting.
class ContextualSuggestionsMetricsReporter
    : public ContextualSuggestionsReporter {
 public:
  ContextualSuggestionsMetricsReporter();
  ~ContextualSuggestionsMetricsReporter() override;

  // ContextualSuggestionsReporter
  void SetupForPage(const std::string& url, ukm::SourceId source_id) override;
  void RecordEvent(ContextualSuggestionsEvent event) override;
  void Flush() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(ContextualSuggestionsMetricsReporterTest, BaseTest);

  // Records UMA metrics for this event.
  void RecordUmaMetrics(ContextualSuggestionsEvent event);

  // Reset UMA metrics.
  void ResetUma();

  // Internal UMA state data.
  // Whether the sheet ever peeked.
  bool sheet_peeked_;
  // Whether the sheet was ever opened.
  bool sheet_opened_;
  // Whether the sheet was closed.
  bool sheet_closed_;
  // Whether any suggestion was downloaded.
  bool any_suggestion_downloaded_;
  // Whether any suggestion was taken.
  bool any_suggestion_taken_;

  // The current UKM entry.
  std::unique_ptr<ContextualSuggestionsUkmEntry> ukm_entry_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsMetricsReporter);
};

using ReportFetchMetricsCallback = base::RepeatingCallback<void(
    contextual_suggestions::ContextualSuggestionsEvent event)>;

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_METRICS_REPORTER_H_
