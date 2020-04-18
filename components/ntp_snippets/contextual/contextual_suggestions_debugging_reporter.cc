// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_suggestions_debugging_reporter.h"

#include <algorithm>

namespace {
static constexpr int CACHE_SIZE = 5;
}  // namespace

namespace contextual_suggestions {

ContextualSuggestionsDebuggingReporter::
    ContextualSuggestionsDebuggingReporter() = default;

ContextualSuggestionsDebuggingReporter::
    ~ContextualSuggestionsDebuggingReporter() = default;

const std::vector<ContextualSuggestionsDebuggingEvent>&
ContextualSuggestionsDebuggingReporter::GetEvents() const {
  return events_;
}

void ContextualSuggestionsDebuggingReporter::SetupForPage(
    const std::string& url,
    ukm::SourceId source_id) {
  current_event_ = ContextualSuggestionsDebuggingEvent();
  current_event_.url = url;
}

void ContextualSuggestionsDebuggingReporter::RecordEvent(
    ContextualSuggestionsEvent event) {
  switch (event) {
    case FETCH_DELAYED:
    case FETCH_REQUESTED:
    case FETCH_ERROR:
    case FETCH_SERVER_BUSY:
    case FETCH_BELOW_THRESHOLD:
    case FETCH_EMPTY:
    case FETCH_COMPLETED:
      return;
    case UI_PEEK_REVERSE_SCROLL:
      current_event_.sheet_peeked = true;
      return;
    case UI_OPENED:
      current_event_.sheet_opened = true;
      return;
    case UI_DISMISSED_WITHOUT_OPEN:
    case UI_DISMISSED_AFTER_OPEN:
      current_event_.sheet_closed = true;
      return;
    case SUGGESTION_DOWNLOADED:
      current_event_.any_suggestion_downloaded = true;
      return;
    case SUGGESTION_CLICKED:
      current_event_.any_suggestion_taken = true;
      return;
    case UI_CLOSED_OBSOLETE:
      NOTREACHED() << "Obsolete event, do not use!";
      return;
    default:
      NOTREACHED() << "Unexpected event, not correctly handled!";
      return;
  }
}

void ContextualSuggestionsDebuggingReporter::Flush() {
  // Check if we've already sent an event with this url to the cache. If so,
  // remove it before adding another one.
  const std::string current_url = current_event_.url;
  std::remove_if(events_.begin(), events_.end(),
                 [current_url](ContextualSuggestionsDebuggingEvent event) {
                   return current_url == event.url;
                 });
  events_.push_back(current_event_);

  // If the cache is too large, then remove the least recently used.
  if (events_.size() > CACHE_SIZE)
    events_.erase(events_.begin());
}

}  // namespace contextual_suggestions