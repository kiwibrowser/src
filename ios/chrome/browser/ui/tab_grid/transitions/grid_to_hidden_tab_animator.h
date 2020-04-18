// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TO_HIDDEN_TAB_ANIMATOR_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TO_HIDDEN_TAB_ANIMATOR_H_

#import <UIKit/UIKit.h>

// Animator object used when the active tab in the tab grid isn't currently
// visible.
@interface GridToHiddenTabAnimator
    : NSObject<UIViewControllerAnimatedTransitioning>

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TO_HIDDEN_TAB_ANIMATOR_H_
