// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_WINDOW_PROPERTY_MANAGER_WIN_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_WINDOW_PROPERTY_MANAGER_WIN_H_

#include <memory>

#include "base/macros.h"
#include "components/prefs/pref_change_registrar.h"

class BrowserView;

// This class is resposible for updating the app id and relaunch details of a
// browser frame.
class BrowserWindowPropertyManager {
 public:
  virtual ~BrowserWindowPropertyManager();

  static std::unique_ptr<BrowserWindowPropertyManager>
  CreateBrowserWindowPropertyManager(BrowserView* view, HWND hwnd);

 private:
  BrowserWindowPropertyManager(BrowserView* view, HWND hwnd);

  void UpdateWindowProperties();
  void OnProfileIconVersionChange();

  PrefChangeRegistrar profile_pref_registrar_;
  BrowserView* view_;
  const HWND hwnd_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWindowPropertyManager);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_WINDOW_PROPERTY_MANAGER_WIN_H_
