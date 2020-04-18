// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_CELL_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_CELL_H_

#import <UIKit/UIKit.h>

namespace web {
class NavigationItem;
}

// Table cell used in TabHistoryViewController.
@interface TabHistoryCell : UICollectionViewCell
@property(assign, nonatomic) const web::NavigationItem* item;
@property(strong, nonatomic, readonly) UILabel* titleLabel;
@end

// Header for a section of TabHistoryCells.
@interface TabHistorySectionHeader : UICollectionReusableView
@property(strong, nonatomic, readonly) UIImageView* iconView;
@property(strong, nonatomic, readonly) UIView* lineView;
@end

// Footer for a section of TabHistoryCells.
@interface TabHistorySectionFooter : UICollectionReusableView
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_CELL_H_
