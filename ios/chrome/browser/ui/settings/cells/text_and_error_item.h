// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_TEXT_AND_ERROR_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_TEXT_AND_ERROR_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// TextAndErrorItem: Displays a text label and might containg an accessory type.
// It might also display an error icon at the right side of the cell if the
// shouldDisplayError flag is set to true.
@interface TextAndErrorItem : CollectionViewItem

// Item text.
@property(nonatomic, copy) NSString* text;

// Boolean to display or hide the error icon.
@property(nonatomic, assign) BOOL shouldDisplayError;

// The accessory type for the represented cell.
@property(nonatomic) MDCCollectionViewCellAccessoryType accessoryType;

// Whether or not the item is enabled.
@property(nonatomic, assign, getter=isEnabled) BOOL enabled;

@end

@interface TextAndErrorCell : MDCCollectionViewCell

// Cell title.
@property(nonatomic, readonly, strong) UILabel* textLabel;

// Error icon that will be displayed on the right side of the cell.
@property(nonatomic, readonly, strong) UIImageView* errorIcon;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_TEXT_AND_ERROR_ITEM_H_
