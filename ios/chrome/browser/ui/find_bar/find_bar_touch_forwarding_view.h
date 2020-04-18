// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_TOUCH_FORWARDING_VIEW_H_
#define IOS_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_TOUCH_FORWARDING_VIEW_H_

#import <UIKit/UIKit.h>

// View that forwards all touch events inside itself to a given |targetView|.
@interface FindBarTouchForwardingView : UIView

// View to forward touch events to.
@property(nonatomic, weak) UIView* targetView;

@end

#endif  // IOS_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_TOUCH_FORWARDING_VIEW_H_
