// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_TABLET_MODE_CLIENT_OBSERVER_H_
#define CHROME_BROWSER_UI_ASH_TABLET_MODE_CLIENT_OBSERVER_H_

// Observer for tablet mode changes inside chrome.
class TabletModeClientObserver {
 public:
  // Fired after the tablet mode has been toggled.
  virtual void OnTabletModeToggled(bool enabled) = 0;

 protected:
  virtual ~TabletModeClientObserver() {}
};

#endif  // CHROME_BROWSER_UI_ASH_TABLET_MODE_CLIENT_OBSERVER_H_
