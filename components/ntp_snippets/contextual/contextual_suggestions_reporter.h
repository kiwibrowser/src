// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_REPORTER_H_
#define COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_REPORTER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace contextual_suggestions {

class ContextualSuggestionsDebuggingReporter;
class ContextualSuggestionsReporter;

// Class producing |ContextualSuggestionsReporter| instances. It enables
// classes like |ContextualContentSuggestionService| to produce metrics
// reporters when needed, e.g. creation of service proxy, without knowing how to
// initialize them.
class ContextualSuggestionsReporterProvider {
 public:
  explicit ContextualSuggestionsReporterProvider(
      std::unique_ptr<ContextualSuggestionsDebuggingReporter>
          debugging_reporter);
  virtual ~ContextualSuggestionsReporterProvider();

  virtual std::unique_ptr<ContextualSuggestionsReporter> CreateReporter();

 private:
  // The debugging reporter is shared between instances of top-level reporter.
  // Since multiple objects need a reference to this, it's kept as a unique
  // pointer, and the raw pointer is given out to sub reporters.
  std::unique_ptr<ContextualSuggestionsDebuggingReporter> debugging_reporter_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsReporterProvider);
};

// NOTE: because this is used for UMA reporting, these values should not be
// changed or reused; new values should be appended immediately before the
// EVENT_MAX value. Make sure to update the histogram enum
// (ContextualSuggestions.Event in enums.xml) accordingly!
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: (
// org.chromium.chrome.browser.contextual_suggestions)
enum ContextualSuggestionsEvent {
  // Indicates that this state is not initialized.
  // Should never be intentionally recorded, just used as a default value.
  UNINITIALIZED = 0,
  // Records that fetching suggestions has been delayed on the client side.
  FETCH_DELAYED = 1,
  // The fetch request has been made but a response has not yet been received.
  FETCH_REQUESTED = 2,
  // The fetch response has been received, but there was some error.
  FETCH_ERROR = 3,
  // The fetch response indicates that the server was too busy to return any
  // suggestions.
  FETCH_SERVER_BUSY = 4,
  // The fetch response includes suggestions but they were all below the
  // confidence threshold needed to show them to the user.
  FETCH_BELOW_THRESHOLD = 5,
  // The fetch response has been received and parsed, but there were no
  // suggestions.
  FETCH_EMPTY = 6,
  // The fetch response has been received and there are suggestions to show.
  FETCH_COMPLETED = 7,
  // The UI was shown in the "peeking bar" state, triggered by a reverse-scroll.
  // If new gestures are added to trigger the peeking sheet then a new event
  // should be added to this list.
  UI_PEEK_REVERSE_SCROLL = 8,
  // The UI sheet was opened.
  UI_OPENED = 9,
  // The UI was closed. General event for closed/dismissed, now obsolete.
  UI_CLOSED_OBSOLETE = 10,
  // A suggestion was downloaded.
  SUGGESTION_DOWNLOADED = 11,
  // A suggestion was taken, either with a click, or opened in a separate tab.
  SUGGESTION_CLICKED = 12,
  // The UI was dismissed without ever being opened. This means the sheet was
  // closed while peeked before ever being expanded.
  UI_DISMISSED_WITHOUT_OPEN = 13,
  // The UI was dismissed after having been opened. This means the sheet was
  // closed from any position after it was expanded at least once.
  UI_DISMISSED_AFTER_OPEN = 14,
  // Special name that marks the maximum value in an Enum used for UMA.
  // https://cs.chromium.org/chromium/src/tools/metrics/histograms/README.md.
  kMaxValue = UI_DISMISSED_AFTER_OPEN,
};

// Tracks various metrics based on reports of events that take place
// within the Contextual Suggestions component.
//
// For example:
// Java UI -> ContextualSuggestionsBridge#reportEvent(
//     /* web_contents */, @ContextualSuggestionsEvent int eventId) -> native
//    -> ContextualContentSuggestionsService#reportEvent(
//          ukm::GetSourceIdForWebContentsDocument(web_contents), eventId)) ->
// if(!reporter) {
//   ContextualSuggestionsReporter* reporter =
//     new ContextualSuggestionsMetricsReporter();
// }
// std::string url;
// ukm::SourceId source_id_for_web_contents;
// reporter->SetupForPage(url, source_id_for_web_contents);
// reporter->RecordEvent(FETCH_REQUESTED);
// ...
//
// if (!my_results)
//   reporter->RecordEvent(FETCH_ERROR);
// else if (my_result.size() == 0)
//   reporter->RecordEvent(FETCH_EMPTY);
// else
//   reporter->RecordEvent(FETCH_COMPLETED);
// ...
// When the UI shows:
// reporter->RecordEvent(UI_PEEK_SHOWN);
// Make sure data is flushed when leaving the page:
// reporter->Flush();
class ContextualSuggestionsReporter {
 public:
  ContextualSuggestionsReporter();
  virtual ~ContextualSuggestionsReporter();

  // Sets up the page with the given |source_id| for event reporting.
  // All subsequent RecordEvent calls will apply to this page
  virtual void SetupForPage(const std::string& url,
                            ukm::SourceId source_id) = 0;

  // Reports that an event occurred for the current page.
  // Some data is not written immediately, but will be written when |Reset| is
  // called.
  virtual void RecordEvent(ContextualSuggestionsEvent event) = 0;

  // Flushes all data staged using |RecordEvent|.
  // This is required before a subsequent call to |SetupForPage|, and can be
  // called multiple times.
  virtual void Flush() = 0;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsReporter);
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_NTP_SNIPPETS_CONTEXTUAL_CONTEXTUAL_SUGGESTIONS_REPORTER_H_
