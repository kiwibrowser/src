// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_ACCOUNT_SIGNIN_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_ACCOUNT_SIGNIN_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// AccountSignInItem is an Item that displays an Image, a title text label and a
// detail text label.
// This is intended to be used as an sign in Item which contains a default
// avatar and information
// letting the user know that they are not signed in, and that tapping on the
// Item will allow them
// to authenticate and sign in.

@interface AccountSignInItem : CollectionViewItem

// Item image.
@property(nonatomic, copy) UIImage* image;

@end

// Cell representation for AccountSignInItem.
@interface AccountSignInCell : MDCCollectionViewCell

// Cell title.
@property(nonatomic, readonly, strong) UILabel* textLabel;
// Cell subtitle.
@property(nonatomic, readonly, strong) UILabel* detailTextLabel;
// Cell imageView.
@property(nonatomic, strong) UIImageView* imageView;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_ACCOUNT_SIGNIN_ITEM_H_
