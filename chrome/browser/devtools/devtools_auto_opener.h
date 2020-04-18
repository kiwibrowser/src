// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVTOOLS_AUTO_OPENER_H_
#define CHROME_BROWSER_DEVTOOLS_DEVTOOLS_AUTO_OPENER_H_

#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class DevToolsAutoOpener : public TabStripModelObserver {
 public:
  DevToolsAutoOpener();
  ~DevToolsAutoOpener() override;

 private:
  // TabStripModelObserver overrides.
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;

  BrowserTabStripTracker browser_tab_strip_tracker_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsAutoOpener);
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVTOOLS_AUTO_OPENER_H_
