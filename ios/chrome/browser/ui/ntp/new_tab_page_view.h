// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_VIEW_H_
#define IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_VIEW_H_

#import <UIKit/UIKit.h>

@class NewTabPageBar;

// Container view for the new tab page so that the subviews don't get directly
// accessed from the view controller.
@interface NewTabPageView : UIView
@property(nonatomic, weak, readonly) NewTabPageBar* tabBar;
@property(nonatomic, weak) UIView* contentView;
@property(nonatomic, weak) UICollectionView* contentCollectionView;
// Safe area to be used for toolbar. Once the view is part of the view hierarchy
// and has its own safe area set, this is equal to safeAreaInsets. But as a
// snapshot of the view is taken before it is inserted in the view hierarchy,
// this property needs to be set to what would be the safe area after being
// inserted in the view hierarchy, before the snapshot is taken.
@property(nonatomic, assign) UIEdgeInsets safeAreaInsetForToolbar;

- (instancetype)initWithFrame:(CGRect)frame
                    andTabBar:(NewTabPageBar*)tabBar NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

// initWithCoder would only be needed for building this view through .xib files.
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NEW_TAB_PAGE_VIEW_H_
