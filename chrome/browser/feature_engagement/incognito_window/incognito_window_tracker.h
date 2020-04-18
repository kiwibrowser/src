// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_H_

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "ui/views/widget/widget_observer.h"

class IncognitoWindowPromoBubbleView;

namespace feature_engagement {

// The IncognitoWindowTracker provides a backend for displaying in-product help
// for the incognito window. IncognitoWindowTracker is the interface through
// which the event constants for the IncognitoWindow feature can be be altered.
// Once all of the event constants are met, IncognitoWindowTracker calls for the
// IncognitoWindowPromo to be shown, along with recording when the
// IncognitoWindowPromo is dismissed. The requirements to show the
// IncognitoWindowPromo are as follows:
//
// - At least two hours of observed session time have elapsed.
// - The user has never opened incognito window through any means.
// - The user has cleared browsing data.
class IncognitoWindowTracker : public FeatureTracker,
                               public views::WidgetObserver {
 public:
  explicit IncognitoWindowTracker(Profile* profile);

  // Alerts the incognito window tracker that an incognito window was opened.
  void OnIncognitoWindowOpened();
  // Alerts the incognito window tracker that browsing history was deleted.
  void OnBrowsingDataCleared();
  // Clears the flag for whether there is any in-product help being displayed.
  void OnPromoClosed();
  // Shows |incognito_promo_|.
  void ShowPromo();

 protected:
  ~IncognitoWindowTracker() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(IncognitoWindowTrackerBrowserTest, ShowPromo);
  FRIEND_TEST_ALL_PREFIXES(IncognitoWindowTrackerEventTest,
                           TestOnSessionTimeMet);
  FRIEND_TEST_ALL_PREFIXES(IncognitoWindowTrackerTest, TestShouldNotShowPromo);
  FRIEND_TEST_ALL_PREFIXES(IncognitoWindowTrackerTest, TestShouldShowPromo);

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  IncognitoWindowPromoBubbleView* incognito_promo() { return incognito_promo_; }

  // FeatureTracker:
  void OnSessionTimeMet() override;

  // Promotional UI that appears next to the AppMenuButton and encourages its
  // use. Owned by its NativeWidget.
  IncognitoWindowPromoBubbleView* incognito_promo_ = nullptr;

  // Observes the |incognito_promo_|'s Widget. Used to tell whether the promo
  // is open and is called back when it closes.
  ScopedObserver<views::Widget, WidgetObserver> incognito_promo_observer_;

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_H_
