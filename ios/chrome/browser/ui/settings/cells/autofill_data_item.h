// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_AUTOFILL_DATA_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_AUTOFILL_DATA_ITEM_H_

#import <UIKit/UIKit.h>

#include <string>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// Item for autofill profile (address) or credit card.
@interface AutofillDataItem : CollectionViewItem

// Only deletable items will enter edit mode.
@property(nonatomic, assign, getter=isDeletable) BOOL deletable;

// The GUID used by the PersonalDataManager to identify data elements (e.g.
// profiles and credit cards).
@property(nonatomic, assign) std::string GUID;

@property(nonatomic, copy) NSString* text;
@property(nonatomic, copy) NSString* leadingDetailText;
@property(nonatomic, copy) NSString* trailingDetailText;

// The accessory type for the represented cell.
@property(nonatomic) MDCCollectionViewCellAccessoryType accessoryType;

@end

// Cell for autofill data with two leading text labels and one trailing text
// label.
@interface AutofillDataCell : MDCCollectionViewCell

@property(nonatomic, readonly, strong) UILabel* textLabel;
@property(nonatomic, readonly, strong) UILabel* leadingDetailTextLabel;
@property(nonatomic, readonly, strong) UILabel* trailingDetailTextLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_AUTOFILL_DATA_ITEM_H_
