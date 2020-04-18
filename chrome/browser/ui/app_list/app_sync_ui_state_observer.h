// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_APP_SYNC_UI_STATE_OBSERVER_H_
#define CHROME_BROWSER_UI_APP_LIST_APP_SYNC_UI_STATE_OBSERVER_H_

class AppSyncUIStateObserver {
 public:
  // Invoked when the UI status of app sync has changed.
  virtual void OnAppSyncUIStatusChanged() = 0;

 protected:
  virtual ~AppSyncUIStateObserver() {}
};

#endif  // CHROME_BROWSER_UI_APP_LIST_APP_SYNC_UI_STATE_OBSERVER_H_
