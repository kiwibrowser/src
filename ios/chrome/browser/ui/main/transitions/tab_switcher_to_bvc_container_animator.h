// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_TRANSITIONS_TAB_SWITCHER_TO_BVC_CONTAINER_ANIMATOR_H_
#define IOS_CHROME_BROWSER_UI_MAIN_TRANSITIONS_TAB_SWITCHER_TO_BVC_CONTAINER_ANIMATOR_H_

#import <UIKit/UIKit.h>

@protocol TabSwitcher;

// This class provides an animator that can animate the transition from the tab
// switcher to the BVC container.
@interface TabSwitcherToBVCContainerAnimator
    : NSObject<UIViewControllerAnimatedTransitioning>

// The TabSwitcher to animate.
@property(nonatomic, readwrite, weak) id<TabSwitcher> tabSwitcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_TRANSITIONS_TAB_SWITCHER_TO_BVC_CONTAINER_ANIMATOR_H_
