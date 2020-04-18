// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_DEBUGGING_REPORTER_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_DEBUGGING_REPORTER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace contextual_suggestions {

// Models the events being reported to the debugging reporter.
// TODO(wylieb): Timestamp events as they happen.
struct ContextualSuggestionsDebuggingEvent {
  // Name of the url.
  std::string url;

  // Whether any suggestion was downloaded.
  bool any_suggestion_downloaded = false;

  // Whether any suggestion was taken.
  bool any_suggestion_taken = false;

  // Whether the sheet was closed.
  bool sheet_closed = false;

  // Whether the sheet was ever opened.
  bool sheet_opened = false;

  // Whether the sheet ever peeked.
  bool sheet_peeked = false;
};

// Reporter specialized for caching information for debugging purposes.
class ContextualSuggestionsDebuggingReporter
    : public ContextualSuggestionsReporter {
 public:
  ContextualSuggestionsDebuggingReporter();
  ~ContextualSuggestionsDebuggingReporter() override;

  // Get all events currently in the buffer.
  const std::vector<ContextualSuggestionsDebuggingEvent>& GetEvents() const;

  // ContextualSuggestionsReporter
  void SetupForPage(const std::string& url, ukm::SourceId source_id) override;
  void RecordEvent(
      contextual_suggestions::ContextualSuggestionsEvent event) override;
  void Flush() override;

 private:
  std::vector<ContextualSuggestionsDebuggingEvent> events_;
  ContextualSuggestionsDebuggingEvent current_event_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsDebuggingReporter);
};
}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_DEBUGGING_REPORTER_H_
