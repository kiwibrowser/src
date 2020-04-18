// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_TAB_USAGE_RECORDER_H_
#define CHROME_BROWSER_METRICS_TAB_USAGE_RECORDER_H_

#include "base/macros.h"
#include "chrome/browser/metrics/tab_reactivation_tracker.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace metrics {

// This class is used to record metrics about tab reactivation. Specifically,
// it records a histogram value everytime a tab is deactivated or reactivated.
// The ratio of both can be calculated for tabs with different properties (e.g.
// If the tab is pinned to the tab strip) to see if they are correlated with
// higher probability of tab reactivation.
class TabUsageRecorder : public TabReactivationTracker::Delegate,
                         public TabStripModelObserver {
 public:
  // Needs to be public for DEFINE_WEB_CONTENTS_USER_DATA_KEY.
  class WebContentsData;

  // Starts recording tab usage for all browsers.
  static void InitializeIfNeeded();

  // TabReactivationTracker::Delegate:
  void OnTabDeactivated(content::WebContents* contents) override;
  void OnTabReactivated(content::WebContents* contents) override;

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;

 private:
  TabUsageRecorder();
  ~TabUsageRecorder() override;

  // Returns the WebContentsData associated with |contents|, creating one if it
  // doesn't exist.
  WebContentsData* GetWebContentsData(content::WebContents* contents);

  TabReactivationTracker tab_reactivation_tracker_;
  BrowserTabStripTracker browser_tab_strip_tracker_;

  DISALLOW_COPY_AND_ASSIGN(TabUsageRecorder);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_TAB_USAGE_RECORDER_H_
