// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_ENCRYPTION_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_ENCRYPTION_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// Item displaying possible options in the Sync Encryption screen.
@interface EncryptionItem : CollectionViewItem

// The accessory type for the represented cell.
@property(nonatomic) MDCCollectionViewCellAccessoryType accessoryType;

// The text to display.
@property(nonatomic, copy) NSString* text;

// Whether or not the cell is enabled.  Disabled cells are drawn with dimmed
// text.
@property(nonatomic, assign, getter=isEnabled) BOOL enabled;

@end

// The cell associated to |EncryptionCell|.
@interface EncryptionCell : MDCCollectionViewCell

// UILabel corresponding to |text| from the item.
@property(nonatomic, readonly, strong) UILabel* textLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_ENCRYPTION_ITEM_H_
