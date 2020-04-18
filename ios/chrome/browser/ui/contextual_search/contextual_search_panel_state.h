// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_PANEL_STATE_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_PANEL_STATE_H_

#import <UIKit/UIKit.h>

namespace ContextualSearch {

// Possible states (static positions) of the panel.
typedef NS_ENUM(NSInteger, PanelState) {
  // TODO(crbug.com/546210): Rename to match Android implementation.
  // Ordering matters for these values.
  UNDEFINED = -1,
  DISMISSED,   // (CLOSED) Offscreen
  PEEKING,     // (PEEKED) Showing a small amount at the bottom of the screen
  PREVIEWING,  // (EXPANDED) Panel covers 2/3 of the tab.
  COVERING,    // (MAXIMIZED) Panel covers entire tab.
};

}  // namespace ContextualSearch

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_PANEL_STATE_H_
