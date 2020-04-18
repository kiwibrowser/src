// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTHENTICATION_ACCOUNT_CONTROL_ITEM_H_
#define IOS_CHROME_BROWSER_UI_AUTHENTICATION_ACCOUNT_CONTROL_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// Item for account collection view and sign-in confirmation view.
@interface AccountControlItem : CollectionViewItem

@property(nonatomic, strong) UIImage* image;
@property(nonatomic, copy) NSString* text;
@property(nonatomic, copy) NSString* detailText;
@property(nonatomic, assign) BOOL shouldDisplayError;

// The accessory type for the represented cell.
@property(nonatomic) MDCCollectionViewCellAccessoryType accessoryType;

@end

// Cell for account settings view with a leading imageView, title text label,
// and detail text label. The imageView is top-leading aligned.
@interface AccountControlCell : MDCCollectionViewCell

@property(nonatomic, readonly, strong) UIImageView* imageView;
@property(nonatomic, readonly, strong) UILabel* textLabel;
@property(nonatomic, readonly, strong) UILabel* detailTextLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTHENTICATION_ACCOUNT_CONTROL_ITEM_H_
