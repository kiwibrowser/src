// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_OBSERVER_H_

#include "base/macros.h"

class Browser;
class BrowserList;

// Interface for observing modifications of a BrowserList.
class BrowserListObserver {
 public:
  BrowserListObserver();
  virtual ~BrowserListObserver();

  // Invoked after |browser| has been added to |browser_list|.
  virtual void OnBrowserCreated(BrowserList* browser_list, Browser* browser);

  // Invoked before |browser| is removed from |browser_list|.
  virtual void OnBrowserRemoved(BrowserList* browser_list, Browser* browser);

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserListObserver);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_OBSERVER_H_
