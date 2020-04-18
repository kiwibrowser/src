// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_PLACEHOLDER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_PLACEHOLDER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tabs/requirements/tab_strip_fold_animation.h"

// Used has a placeholder for the tab strip view during transition animations.
@interface TabStripPlaceholderView : UIView<TabStripFoldAnimation>
@end

#endif  // IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_PLACEHOLDER_VIEW_H_
