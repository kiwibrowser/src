// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_GET_APP_MENU_ANCHOR_POINT_H_
#define CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_GET_APP_MENU_ANCHOR_POINT_H_

#include "ui/gfx/geometry/point.h"

class Browser;

// Returns the point to which bubbles should be anchored for the app menu of
// |browser|.
gfx::Point GetAppMenuAnchorPoint(Browser* browser);

#endif  // CHROME_BROWSER_UI_VIEWS_RELAUNCH_NOTIFICATION_GET_APP_MENU_ANCHOR_POINT_H_
