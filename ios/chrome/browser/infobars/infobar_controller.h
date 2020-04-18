// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTROLLER_H_
#define IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/infobars/infobar_view_sizing_delegate.h"

class InfoBarControllerDelegate;
@protocol InfoBarViewSizing;

// InfoBar for iOS acts as a UIViewController for InfoBarView.
@interface InfoBarController : NSObject<InfoBarViewSizingDelegate>

// Creates a view and lays out all the infobar elements in it. Will not add
// it as a subview yet.
- (void)layoutForFrame:(CGRect)bounds;

// Detaches view from its delegate.
// After this function is called, no user interaction can be handled.
- (void)detachView;

// Returns the actual height in pixels of this infobar instance.
- (int)barHeight;

// Adjusts visible portion of this infobar.
- (void)onHeightRecalculated:(int)newHeight;

// Removes the view.
- (void)removeView;

// Accesses the view.
- (UIView<InfoBarViewSizing>*)view;

@property(nonatomic, assign) InfoBarControllerDelegate* delegate;  // weak

@end

#endif  // IOS_CHROME_BROWSER_INFOBARS_INFOBAR_CONTROLLER_H_
