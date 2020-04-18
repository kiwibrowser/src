// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_OBSERVER_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_OBSERVER_H_

class AppMenuObserver {
 public:
  // Invoked when the AppMenu is about to be destroyed (from its destructor).
  virtual void AppMenuDestroyed() = 0;

 protected:
  virtual ~AppMenuObserver() {}
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_APP_MENU_OBSERVER_H_
