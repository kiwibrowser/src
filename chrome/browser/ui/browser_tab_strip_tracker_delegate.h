// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_TAB_STRIP_TRACKER_DELEGATE_H_
#define CHROME_BROWSER_UI_BROWSER_TAB_STRIP_TRACKER_DELEGATE_H_

#include "chrome/browser/ui/browser_list_observer.h"

class Browser;

class BrowserTabStripTrackerDelegate {
 public:
  // Called to determine if the supplied Browser should be tracked. See
  // BrowserTabStripTracker for details.
  virtual bool ShouldTrackBrowser(Browser* browser) = 0;

 protected:
  virtual ~BrowserTabStripTrackerDelegate() {}
};

#endif  // CHROME_BROWSER_UI_BROWSER_TAB_STRIP_TRACKER_DELEGATE_H_
