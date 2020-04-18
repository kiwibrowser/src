// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_LIST_OBSERVER_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_LIST_OBSERVER_H_

#include "base/macros.h"

@class TabModel;

namespace ios {
class ChromeBrowserState;
}  // namespace ios

// Interface for getting notified when TabModels get associated/dissociated
// to/from browser states.
class TabModelListObserver {
 public:
  TabModelListObserver() = default;
  virtual ~TabModelListObserver() = default;

  // Called when |tab_model| is associated to |browser_state|.
  virtual void TabModelRegisteredWithBrowserState(
      TabModel* tab_model,
      ios::ChromeBrowserState* browser_state) = 0;

  // Called when the |tab_model| is dissociated from |browser_state|.
  virtual void TabModelUnregisteredFromBrowserState(
      TabModel* tab_model,
      ios::ChromeBrowserState* browser_state) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabModelListObserver);
};

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_LIST_OBSERVER_H_
