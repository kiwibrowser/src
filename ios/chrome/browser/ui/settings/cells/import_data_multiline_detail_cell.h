// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_IMPORT_DATA_MULTILINE_DETAIL_CELL_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_IMPORT_DATA_MULTILINE_DETAIL_CELL_H_

#import <UIKit/UIKit.h>

#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// ImportDataMultilineDetailCell implements an MDCCollectionViewCell
// subclass containing two text labels: a "main" label and a "detail" label.
// The two labels are laid out on top of each other. The detail text can span
// multiple lines.
// This is to be used with a CollectionViewDetailItem.
@interface ImportDataMultilineDetailCell : MDCCollectionViewCell

// UILabels corresponding to |text| and |detailText| from the item.
@property(nonatomic, readonly, strong) UILabel* textLabel;
@property(nonatomic, readonly, strong) UILabel* detailTextLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_IMPORT_DATA_MULTILINE_DETAIL_CELL_H_
