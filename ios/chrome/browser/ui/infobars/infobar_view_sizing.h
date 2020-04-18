// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_INFOBARS_INFOBAR_VIEW_SIZING_H_
#define IOS_CHROME_BROWSER_UI_INFOBARS_INFOBAR_VIEW_SIZING_H_

#import <UIKit/UIKit.h>

@protocol InfoBarViewSizingDelegate;

// Protocol implemented by UIView subclasses representing an infobar. It has
// information about the infobar's visible height and a reference to the
// delegate that gets notified of infobar's height changes.
@protocol InfoBarViewSizing

// How much of the infobar (in points) is visible (e.g., during showing/hiding
// animation).
@property(nonatomic, assign) CGFloat visibleHeight;

// The delegate that gets notified of infobar's height changes.
@property(nonatomic, weak) id<InfoBarViewSizingDelegate> sizingDelegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_INFOBARS_INFOBAR_VIEW_SIZING_H_
