// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"

@class TabSwitcherCache;

@interface TabSwitcherPanelView : UIView

@property(nonatomic, readonly) UICollectionView* collectionView;

- (instancetype)initWithSessionType:(TabSwitcherSessionType)sessionType
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Updates the collectionView's layout to ensure that the optimal amount of tabs
// are displayed. The completion block is called at the end of the layout
// update.
- (void)updateCollectionLayoutWithCompletion:(void (^)(void))completion;

// Returns the size of the cells displayed in the collectionView.
- (CGSize)cellSize;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_VIEW_H_
