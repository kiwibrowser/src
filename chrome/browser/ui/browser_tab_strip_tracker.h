// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_TAB_STRIP_TRACKER_H_
#define CHROME_BROWSER_UI_BROWSER_TAB_STRIP_TRACKER_H_

#include <set>

#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"

class BrowserTabStripTrackerDelegate;
class TabStripModelObserver;

// BrowserTabStripTracker is useful when you want to attach a
// TabStripModelObserver to a subset of the available browsers, as well as
// tracking new Browsers as they are added.
//
// To constrain the set of Browsers to track use a
// BrowserTabStripTrackerDelegate. BrowserTabStripTracker queries the delegate
// for each Browser to determine if the Browser should be tracked. A null
// delegate indicates all Browsers should be observed.
//
// If you are interested in BrowserListObserver functions specify a
// BrowserListObserver in the constructor. OnBrowserAdded() and
// OnBrowserRemoved() are only called if the delegate indicates the browser
// should be tracked.
class BrowserTabStripTracker : public BrowserListObserver {
 public:
  // See class description for details. You only need specify a
  // TabStripModelObserver. |delegate| and |browser_list_observer| are
  // optional.
  BrowserTabStripTracker(TabStripModelObserver* tab_strip_model_observer,
                         BrowserTabStripTrackerDelegate* delegate,
                         BrowserListObserver* browser_list_observer);
  ~BrowserTabStripTracker() override;

  // Starts tracking BrowserList for changes and additionally observes the
  // existing Browsers. If there is a BrowserTabStripTrackerDelegate it is
  // called to determine if the Browser should be observed. If an existing
  // Browser should be observed TabInsertedAt() is called for any existing tabs.
  // If a delegate needs to differentiate between Browsers observed by way of
  // Init() vs. a Browser added after the fact use
  // is_processing_initial_browsers().
  void Init();

  // Returns true if processing an existing Browser in Init().
  bool is_processing_initial_browsers() const {
    return is_processing_initial_browsers_;
  }

  // Stops observing the current set of observed browsers and calls
  // BrowserListObserver::OnBrowserRemoved().
  void StopObservingAndSendOnBrowserRemoved();

 private:
  using Browsers = std::set<Browser*>;

  // Returns true if a TabStripModelObserver should be added to |browser|.
  bool ShouldTrackBrowser(Browser* browser);

  // If ShouldTrackBrowser() returns true for |browser| then a
  // TabStripModelObserver is attached.
  void MaybeTrackBrowser(Browser* browser);

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserRemoved(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;

  TabStripModelObserver* tab_strip_model_observer_;
  BrowserTabStripTrackerDelegate* delegate_;
  BrowserListObserver* browser_list_observer_;
  bool is_processing_initial_browsers_;
  Browsers browsers_observing_;

  DISALLOW_COPY_AND_ASSIGN(BrowserTabStripTracker);
};

#endif  // CHROME_BROWSER_UI_BROWSER_TAB_STRIP_TRACKER_H_
