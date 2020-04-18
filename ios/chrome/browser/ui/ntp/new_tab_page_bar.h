// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_H_

#import <UIKit/UIKit.h>

@class NewTabPageBarItem;

@protocol NewTabPageBarDelegate
// Called when new tab page bar item is selected and the selection is changed.
- (void)newTabBarItemDidChange:(NewTabPageBarItem*)selectedItem;
@end

// The bar in the new tab page that switches between the provided choices.
// It also draws a notch pointing to the center of the selected choice to
// create a "speech bubble" effect.
@interface NewTabPageBar : UIView<UIGestureRecognizerDelegate>

@property(nonatomic, strong) NSArray* items;
// Which button is currently selected.
@property(nonatomic, assign) NSUInteger selectedIndex;
// Percentage of the overlay that sits over the tab bar buttons.
@property(nonatomic, assign) CGFloat overlayPercentage;
@property(nonatomic, readonly, strong) NSArray* buttons;
@property(nonatomic, weak) id<NewTabPageBarDelegate> delegate;

// Safe area set by the NTP view. This is used as the safeAreaInsets of this
// view as it needs to be used before the safeAreaInsets is set up.
@property(nonatomic, assign) UIEdgeInsets safeAreaInsetFromNTPView;

// Updates the alpha of the shadow image. When the alpha changes from 0 to 1 or
// 1 to 0, the alpha change is animated.
- (void)setShadowAlpha:(CGFloat)alpha;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_BAR_H_
