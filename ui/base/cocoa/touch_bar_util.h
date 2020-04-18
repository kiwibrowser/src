// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_TOUCH_BAR_UTIL_H
#define UI_BASE_COCOA_TOUCH_BAR_UTIL_H

#import <Cocoa/Cocoa.h>

#include "base/mac/availability.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// The touch bar actions that are being recorded in a histogram. These values
// should not be re-ordered or removed.
enum TouchBarAction {
  BACK = 0,
  FORWARD,
  STOP,
  RELOAD,
  HOME,
  SEARCH,
  STAR,
  NEW_TAB,
  CREDIT_CARD_AUTOFILL,
  TOUCH_BAR_ACTION_COUNT
};

// Logs the sample's UMA metrics into the DefaultTouchBar.Metrics histogram.
UI_BASE_EXPORT void LogTouchBarUMA(TouchBarAction command);

// Returns the NSTouchBar Class.
UI_BASE_EXPORT Class NSTouchBar();

// Returns the NSCustomTouchBarItem Class.
UI_BASE_EXPORT Class NSCustomTouchBarItem();

// Returns the NSGroupTouchBarItem Class.
UI_BASE_EXPORT Class NSGroupTouchBarItem();

// Returns a stylized blue button for the touch bar. The button performs
// |action| from the |target|.
API_AVAILABLE(macosx(10.12.2))
UI_BASE_EXPORT NSButton* GetBlueTouchBarButton(NSString* title,
                                               id target,
                                               SEL action);

// Creates a touch bar identifier with the given |id|.
UI_BASE_EXPORT NSString* GetTouchBarId(NSString* touch_bar_id);

// Creates a touch Bar jtem identifier.
UI_BASE_EXPORT NSString* GetTouchBarItemId(NSString* touch_bar_id,
                                           NSString* item_id);

}  // namespace ui

#endif  // UI_BASE_COCOA_TOUCH_BAR_UTIL_H
