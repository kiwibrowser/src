// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_UKM_ENTRY_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_UKM_ENTRY_H_

#include "base/gtest_prod_util.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace base {
class ElapsedTimer;
}

namespace contextual_suggestions {

FORWARD_DECLARE_TEST(ContextualSuggestionsUkmEntryTest, BinaryOrderTest);

// The state of the Fetcher request.
// This value is written to the "FetchState" UKM metric.
enum class FetchState {
  NOT_STARTED,
  DELAYED,
  REQUESTED,
  ERROR_RESULT,
  SERVER_BUSY,
  BELOW_THRESHOLD,
  EMPTY,
  COMPLETED,
};

// The way that the sheet UI was triggered.
// This value is written to the "TriggerEvent" UKM metric.
enum class TriggerEvent {
  NEVER_SHOWN,
  REVERSE_SCROLL,
};

// Writes a single UKM entry that describes the latest state of the event stream
// monitored through |RecordEventMetrics|.
class ContextualSuggestionsUkmEntry {
 public:
  // Sets up recording of a UKM entry for the given |source_id|.
  explicit ContextualSuggestionsUkmEntry(ukm::SourceId source_id);
  ~ContextualSuggestionsUkmEntry();

  // Updates tracked metrics for the given |event|.
  void RecordEventMetrics(ContextualSuggestionsEvent event);

  // Writes the latest data tracked into a single UKM entry.
  void Flush();

 private:
  FRIEND_TEST_ALL_PREFIXES(ContextualSuggestionsUkmEntryTest, BinaryOrderTest);

  // Starts the elapsed timer if not already started.
  void StartTimerIfNeeded();

  // Stops the elapsed timer if not already stopped.
  void StopTimerIfNeeded();

  // Internal state trackers.
  FetchState fetch_state_;
  TriggerEvent trigger_event_;
  bool closed_from_peek_;
  bool was_sheet_opened_;
  bool any_suggestion_taken_;
  bool any_suggestion_downloaded_;

  // The minimum exponential bucket of the duration in milliseconds that the
  // sheet was viewed before taking action.  A value of 0 means not yet
  // computed.
  int64_t show_duration_exponential_bucket_;

  // Simple timer for how long the sheet is showing.
  std::unique_ptr<base::ElapsedTimer> show_duration_timer_;

  // The UKM identifier for the current URL / page in use.
  ukm::SourceId source_id_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsUkmEntry);
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_UKM_ENTRY_H_
