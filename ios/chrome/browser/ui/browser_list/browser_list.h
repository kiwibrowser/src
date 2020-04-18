// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

class Browser;
class BrowserListObserver;

// BrowserList attaches multiple Browser to a single ChromeBrowserState.
// It allows listening for the addition or removal of Browsers via the
// BrowserListObserver interface.
//
// See src/docs/ios/objects.md for more information.
class BrowserList : public KeyedService {
 public:
  ~BrowserList() override;

  // Returns the number of Browsers in the BrowserList.
  virtual int GetCount() const = 0;

  // Returns whether the specified index is valid.
  virtual int ContainsIndex(int index) const = 0;

  // Returns the Browser at the specified index.
  virtual Browser* GetBrowserAtIndex(int index) const = 0;

  // Returns the index of the specified Browser, or kInvalidIndex if not found.
  virtual int GetIndexOfBrowser(const Browser* browser) const = 0;

  // Creates and returns a new Browser instance.
  virtual Browser* CreateNewBrowser() = 0;

  // Closes the Browser at the specified index.
  virtual void CloseBrowserAtIndex(int index) = 0;

  // Adds/removes |observer| from the list of observers.
  virtual void AddObserver(BrowserListObserver* observer) = 0;
  virtual void RemoveObserver(BrowserListObserver* observer) = 0;

  // Invalid index.
  static constexpr int kInvalidIndex = -1;

 protected:
  BrowserList();

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserList);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_LIST_H_
