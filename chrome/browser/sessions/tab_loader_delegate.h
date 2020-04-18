// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SESSIONS_TAB_LOADER_DELEGATE_H_
#define CHROME_BROWSER_SESSIONS_TAB_LOADER_DELEGATE_H_

#include <memory>

#include "base/time/time.h"

class TabLoaderCallback {
 public:
  // This function will get called to suppress and to allow tab loading. Tab
  // loading is initially enabled.
  virtual void SetTabLoadingEnabled(bool enable_tab_loading) = 0;
};

// TabLoaderDelegate is created once the SessionRestore process is complete and
// the loading of hidden tabs starts.
class TabLoaderDelegate {
 public:
  TabLoaderDelegate() {}
  virtual ~TabLoaderDelegate() {}

  // Create a tab loader delegate. |TabLoaderCallback::SetTabLoadingEnabled| can
  // get called to disable / enable tab loading.
  // The callback object is valid as long as this object exists.
  static std::unique_ptr<TabLoaderDelegate> Create(TabLoaderCallback* callback);

  // Returns the default timeout time after which the first non-visible tab
  // gets loaded if the first (visible) tab did not finish loading.
  virtual base::TimeDelta GetFirstTabLoadingTimeout() const = 0;

  // Returns the default timeout time after which the next tab gets loaded if
  // the previous tab did not finish loading.
  virtual base::TimeDelta GetTimeoutBeforeLoadingNextTab() const = 0;
};

#endif  // CHROME_BROWSER_SESSIONS_TAB_LOADER_DELEGATE_H_
