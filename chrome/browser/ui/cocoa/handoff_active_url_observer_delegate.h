// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_HANDOFF_ACTIVE_URL_OBSERVER_DELEGATE_H_
#define CHROME_BROWSER_UI_COCOA_HANDOFF_ACTIVE_URL_OBSERVER_DELEGATE_H_

namespace content {
class WebContents;
}

// The delegate for a HandoffActiveURLObserver.
class HandoffActiveURLObserverDelegate {
 public:
  // Called when:
  //   1. The most recently focused browser changes.
  //   2. The active tab of the browser changes.
  //   3. After a navigation of the web contents of the active tab.
  // |web_contents| is the WebContents whose VisibleURL is considered the
  // "Active URL" of Chrome.
  virtual void HandoffActiveURLChanged(content::WebContents* web_contents) = 0;

 protected:
  virtual ~HandoffActiveURLObserverDelegate(){};
};

#endif  // CHROME_BROWSER_UI_COCOA_HANDOFF_ACTIVE_URL_OBSERVER_DELEGATE_H_
