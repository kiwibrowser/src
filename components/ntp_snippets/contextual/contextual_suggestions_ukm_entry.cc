// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_ukm_entry.h"

#include <algorithm>

#include "base/timer/elapsed_timer.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "services/metrics/public/cpp/metrics_utils.h"
#include "services/metrics/public/cpp/ukm_builders.h"

namespace contextual_suggestions {

namespace {
// Spacing between buckets for the duration in milliseconds.
// The values recorded will be the min of the bucket in a power of 2 range.
// E.g. 1100 => 1024 since 1024 <= 1100 < 2048.
const double kShowDurationBucketSpacing = 2.0;
const int64_t kMinShowDuration = 1;
}  // namespace

ContextualSuggestionsUkmEntry::ContextualSuggestionsUkmEntry(
    ukm::SourceId source_id)
    : fetch_state_(FetchState::NOT_STARTED),
      trigger_event_(TriggerEvent::NEVER_SHOWN),
      closed_from_peek_(false),
      was_sheet_opened_(false),
      any_suggestion_taken_(false),
      any_suggestion_downloaded_(false),
      show_duration_exponential_bucket_(0),
      show_duration_timer_(nullptr),
      source_id_(source_id) {}

ContextualSuggestionsUkmEntry::~ContextualSuggestionsUkmEntry() {}

void ContextualSuggestionsUkmEntry::RecordEventMetrics(
    ContextualSuggestionsEvent event) {
  switch (event) {
    case UNINITIALIZED:
      NOTREACHED() << "An uninitialized event value was sent!";
      break;
    case FETCH_DELAYED:
      fetch_state_ = FetchState::DELAYED;
      break;
    case FETCH_REQUESTED:
      fetch_state_ = FetchState::REQUESTED;
      break;
    case FETCH_ERROR:
      fetch_state_ = FetchState::ERROR_RESULT;
      break;
    case FETCH_SERVER_BUSY:
      fetch_state_ = FetchState::SERVER_BUSY;
      break;
    case FETCH_BELOW_THRESHOLD:
      fetch_state_ = FetchState::BELOW_THRESHOLD;
      break;
    case FETCH_EMPTY:
      fetch_state_ = FetchState::EMPTY;
      break;
    case FETCH_COMPLETED:
      fetch_state_ = FetchState::COMPLETED;
      break;
    case UI_PEEK_REVERSE_SCROLL:
      trigger_event_ = TriggerEvent::REVERSE_SCROLL;
      StartTimerIfNeeded();
      break;
    case UI_OPENED:
      was_sheet_opened_ = true;
      StartTimerIfNeeded();
      break;
    case UI_CLOSED_OBSOLETE:
      NOTREACHED();
      break;
    case SUGGESTION_DOWNLOADED:
      any_suggestion_downloaded_ = true;
      StopTimerIfNeeded();
      break;
    case SUGGESTION_CLICKED:
      any_suggestion_taken_ = true;
      StopTimerIfNeeded();
      break;
    case UI_DISMISSED_WITHOUT_OPEN:
      closed_from_peek_ = true;
      StopTimerIfNeeded();
      break;
    case UI_DISMISSED_AFTER_OPEN:
      StopTimerIfNeeded();
      break;
  }
}

void ContextualSuggestionsUkmEntry::Flush() {
  if (source_id_ == ukm::kInvalidSourceId)
    return;

  if (show_duration_timer_)
    StopTimerIfNeeded();

  // Keep these writes alphabetical by setter name (matching ukm.xml ordering).
  ukm::builders::ContextualSuggestions builder(source_id_);
  builder.SetAnyDownloaded(any_suggestion_downloaded_)
      .SetAnySuggestionTaken(any_suggestion_taken_)
      .SetClosedFromPeek(closed_from_peek_)
      .SetEverOpened(was_sheet_opened_)
      .SetFetchState(static_cast<int64_t>(fetch_state_))
      .SetShowDurationBucketMin(show_duration_exponential_bucket_)
      .SetTriggerEvent(static_cast<int64_t>(trigger_event_))
      .Record(ukm::UkmRecorder::Get());

  source_id_ = ukm::kInvalidSourceId;
}

void ContextualSuggestionsUkmEntry::StartTimerIfNeeded() {
  // If the timer is already running, don't restart it.
  if (show_duration_timer_)
    return;

  show_duration_timer_.reset(new base::ElapsedTimer());
}

void ContextualSuggestionsUkmEntry::StopTimerIfNeeded() {
  // We should either have a timer or a computed duration from the timer.
  DCHECK(show_duration_timer_ || show_duration_exponential_bucket_ > 0);

  // If we've already computed the duration, or there's no timer to stop, then
  // there's nothing to do.
  if (show_duration_exponential_bucket_ > 0 || !show_duration_timer_)
    return;

  show_duration_exponential_bucket_ = ukm::GetExponentialBucketMin(
      show_duration_timer_->Elapsed().InMillisecondsRoundedUp(),
      kShowDurationBucketSpacing);
  // Ensure a positive value.
  show_duration_exponential_bucket_ =
      std::max(kMinShowDuration, show_duration_exponential_bucket_);
  show_duration_timer_.reset();
}

}  // namespace contextual_suggestions
