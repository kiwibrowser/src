// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_PASSPHRASE_ERROR_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_PASSPHRASE_ERROR_ITEM_H_

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// Item to display an error when the passphrase is incorrect.
@interface PassphraseErrorItem : CollectionViewItem

// The error text. It appears in red.
@property(nonatomic, copy) NSString* text;

@end

// Cell class associated to PassphraseErrorItem.
@interface PassphraseErrorCell : MDCCollectionViewCell

// Label for the error text.
@property(nonatomic, readonly, strong) UILabel* textLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_PASSPHRASE_ERROR_ITEM_H_
