// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/touch_bar_util.h"

#include "base/mac/foundation_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"

namespace ui {

void LogTouchBarUMA(TouchBarAction command) {
  UMA_HISTOGRAM_ENUMERATION("TouchBar.Default.Metrics", command,
                            TOUCH_BAR_ACTION_COUNT);
}

Class NSTouchBar() {
  return NSClassFromString(@"NSTouchBar");
}

Class NSCustomTouchBarItem() {
  return NSClassFromString(@"NSCustomTouchBarItem");
}

Class NSGroupTouchBarItem() {
  return NSClassFromString(@"NSGroupTouchBarItem");
}

NSButton* GetBlueTouchBarButton(NSString* title, id target, SEL action) {
  NSButton* button =
      [NSButton buttonWithTitle:title target:target action:action];
  [button setBezelColor:[NSColor colorWithSRGBRed:0.168
                                            green:0.51
                                             blue:0.843
                                            alpha:1.0]];
  return button;
}

NSString* GetTouchBarId(NSString* touch_bar_id) {
  NSString* chrome_bundle_id =
      base::SysUTF8ToNSString(base::mac::BaseBundleID());
  return [NSString stringWithFormat:@"%@.%@", chrome_bundle_id, touch_bar_id];
}

NSString* GetTouchBarItemId(NSString* touch_bar_id, NSString* item_id) {
  return [NSString
      stringWithFormat:@"%@-%@", GetTouchBarId(touch_bar_id), item_id];
}

}  // namespace ui
