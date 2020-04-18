// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAGE_INFO_ITEM_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAGE_INFO_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// The accessibility identifier for the favicon image view.
extern NSString* const kPageInfoFaviconImageViewID;

// The accessibility identifier for the lock indicator image view.
extern NSString* const kPageInfoLockIndicatorImageViewID;

// PageInfoItem is the model class corresponding to PageInfoCell.
@interface PageInfoItem : CollectionViewItem

// The favicon image to display.
@property(nonatomic, strong) UIImage* pageFavicon;

// The page title text to display.
@property(nonatomic, copy) NSString* pageTitle;

// The page host text to display.
@property(nonatomic, copy) NSString* pageHost;

// Whether or not the connection is secure.
@property(nonatomic, assign, getter=isConnectionSecure) BOOL connectionSecure;

@end

// PageInfoCell implements a MDCCollectionViewCell subclass containing two image
// views displaying the current page's favicon and a lock indicator if
// connection is secure and two text labels representing the current page's
// title and host. The favicon image is laid out on the leading edge of the cell
// while the two labels are laid out trailing the favicon and one on top of the
// other, filling the full width of the cell, if the connection is not secure.
// In the case that the connection is secure, the lock indicator image is placed
// ahead of the host label and trails the favicon image. Labels are truncated as
// needed to fit in the cell.
@interface PageInfoCell : MDCCollectionViewCell

// UILabels containing the page's title and host.
@property(nonatomic, readonly, strong) UILabel* pageTitleLabel;
@property(nonatomic, readonly, strong) UILabel* pageHostLabel;

// UIImageViews containing the page's favicon and lock indicator.
@property(nonatomic, readonly, strong) UIImageView* pageFaviconView;
@property(nonatomic, readonly, strong) UIImageView* pageLockIndicatorView;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CELLS_PAGE_INFO_ITEM_H_
