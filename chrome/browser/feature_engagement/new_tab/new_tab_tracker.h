// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "chrome/browser/feature_engagement/session_duration_updater.h"

namespace feature_engagement {

// The NewTabTracker provides a backend for displaying in-product help for the
// new tab button. NewTabTracker is the interface through which the event
// constants for the NewTab feature can be be altered. Once all of the event
// constants are met, NewTabTracker calls for the NewTabPromo to be shown, along
// with recording when the NewTabPromo is dismissed. The requirements to show
// the NewTabPromo are as follows:
//
// - At least two hours of observed session time have elapsed.
// - The user has never opened another tab through any means.
// - The user has navigated away from the start home screen.
// - The omnibox is in focus, which implies the user is intending on navigating
//   to a new page.
class NewTabTracker : public FeatureTracker {
 public:
  explicit NewTabTracker(Profile* profile);

  // Alerts the new tab tracker that a new tab was opened.
  void OnNewTabOpened();
  // Alerts the new tab tracker that the omnibox has been used.
  void OnOmniboxNavigation();
  // Checks if the promo should be displayed since the omnibox is on focus.
  void OnOmniboxFocused();
  // Clears the flag for whether there is any in-product help being displayed.
  void OnPromoClosed();
  // Shows new tab in-product help promo bubble in last active browser.
  void ShowPromo();

  // Returns whether there was a bubble that was closed. A bubble closes only
  // when it exists.
  void CloseBubble();

 protected:
  ~NewTabTracker() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(NewTabTrackerEventTest, TestOnSessionTimeMet);
  FRIEND_TEST_ALL_PREFIXES(NewTabTrackerTest, TestShouldNotShowPromo);
  FRIEND_TEST_ALL_PREFIXES(NewTabTrackerTest, TestShouldShowPromo);
  FRIEND_TEST_ALL_PREFIXES(NewTabTrackerBrowserTest, TestShowPromo);

  // FeatureTracker:
  void OnSessionTimeMet() override;

  DISALLOW_COPY_AND_ASSIGN(NewTabTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
