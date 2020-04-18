// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_VERSION_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_VERSION_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// Item to display the version of the current build.
@interface VersionItem : CollectionViewItem

// The display string representing the version.
@property(nonatomic, copy) NSString* text;

@end

// Cell class associated to VersionItem.
@interface VersionCell : MDCCollectionViewCell

// Label for the current build version.
@property(nonatomic, readonly, strong) UILabel* textLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_VERSION_ITEM_H_
