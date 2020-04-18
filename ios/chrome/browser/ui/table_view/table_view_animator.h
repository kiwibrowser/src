// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_TABLE_VIEW_ANIMATOR_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_TABLE_VIEW_ANIMATOR_H_

#import <UIKit/UIKit.h>

// TableViewAnimator implements an animation that slides the presented view in
// from the trailing edge of the screen.
@interface TableViewAnimator : NSObject<UIViewControllerAnimatedTransitioning>

// YES if this animator is presenting a view controller, NO if it is dismissing
// one.
@property(nonatomic, assign) BOOL presenting;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_TABLE_VIEW_ANIMATOR_H_
