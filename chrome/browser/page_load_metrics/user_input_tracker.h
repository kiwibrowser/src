// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_USER_INPUT_TRACKER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_USER_INPUT_TRACKER_H_

#include <vector>

#include "base/macros.h"
#include "base/time/time.h"

namespace blink {
class WebInputEvent;
}  // namespace blink

namespace page_load_metrics {

// UserInputTracker keeps track of user input events processed by web pages, and
// allows clients to find and consume those input events. This allows us to
// heuristically attribute user input events to navigations, in order to keep
// track of which page loads and aborts were initiated by a user action.
//
// There are issues with the existing user gesture tracking in Blink and content
// that make it unsuitable for our needs. For example, Blink considers events
// such as navigations that occur within 1 second of a user action event to have
// been initiated by a user action, based on the HTML spec
// (https://html.spec.whatwg.org/multipage/interaction.html#triggered-by-user-activation).
// This can be problematic in cases where a web page issues many navigations in
// rapid succession, e.g. JS code that dispatches new navigation requests in a
// tight loop can result in dozens of programmatically generated navigations
// being user initiated.
//
// Note that UserInputTracker does not keep track of input events processed by
// the browser, such as interactions with the Chrome browser UI (e.g. clicking
// the 'reload' button).
class UserInputTracker {
 public:
  // Only public for tests.
  static constexpr size_t kMaxTrackedEvents = 100;

  // Given a time, round to the nearest rate-limited offset. UserInputTracker
  // rate limits events, such that at most one event will be recorded per every
  // 20ms. RoundToRateLimitedOffset round a TimeTicks down to its nearest whole
  // 20ms.
  static base::TimeTicks RoundToRateLimitedOffset(base::TimeTicks time);

  UserInputTracker();
  ~UserInputTracker();

  void OnInputEvent(const blink::WebInputEvent& event);

  // Attempts to find the most recent user input event before the given time,
  // and, if that input event exists, consumes all events up to that
  // event. Returns whether an input event before the given time was found and
  // consumed.
  bool FindAndConsumeInputEventsBefore(base::TimeTicks time);

  // Finds the time of the most recent user input event before the given time,
  // or a null TimeTicks if there are no user input events before the given
  // time. Consumers of this class should use
  // FindAndConsumeInputEventsBefore. This method is public only for testing.
  base::TimeTicks FindMostRecentUserInputEventBefore(base::TimeTicks time);

 private:
  void RemoveInputEventsUpToInclusive(base::TimeTicks cutoff);

  static base::TimeDelta GetOldEventThreshold();

  std::vector<base::TimeTicks> sorted_event_times_;
  base::TimeTicks most_recent_consumed_time_;

  DISALLOW_COPY_AND_ASSIGN(UserInputTracker);
};

}  // namespace page_load_metrics

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_USER_INPUT_TRACKER_H_
