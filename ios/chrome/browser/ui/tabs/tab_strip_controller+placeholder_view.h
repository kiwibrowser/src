// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_CONTROLLER_PLACEHOLDER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_CONTROLLER_PLACEHOLDER_VIEW_H_

#import "ios/chrome/browser/ui/tabs/tab_strip_controller.h"

@protocol TabStripFoldAnimation;

// This category is used by the tab switcher to start fold/unfold animations
// on the tab strip controller.
@interface TabStripController (PlaceholderView)

// Returns a tab strip placeholder view created from the current state of the
// tab strip controller. It is used to animate transitions with the tab strip.
- (UIView<TabStripFoldAnimation>*)placeholderView;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_CONTROLLER_PLACEHOLDER_VIEW_H_
