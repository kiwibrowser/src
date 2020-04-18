// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_TRANSITIONS_BVC_CONTAINER_TO_TAB_SWITCHER_ANIMATOR_H_
#define IOS_CHROME_BROWSER_UI_MAIN_TRANSITIONS_BVC_CONTAINER_TO_TAB_SWITCHER_ANIMATOR_H_

#import <UIKit/UIKit.h>

@protocol TabSwitcher;

@interface BVCContainerToTabSwitcherAnimator
    : NSObject<UIViewControllerAnimatedTransitioning>

// The TabSwitcher to animate.
@property(nonatomic, readwrite, weak) id<TabSwitcher> tabSwitcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_TRANSITIONS_BVC_CONTAINER_TO_TAB_SWITCHER_ANIMATOR_H_
