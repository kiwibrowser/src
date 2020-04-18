// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_CELLS_STORAGE_SWITCH_ITEM_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_CELLS_STORAGE_SWITCH_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// StorageSwitchItem is the model class corresponding to StorageSwitchCell.
@interface StorageSwitchItem : CollectionViewItem

// The current state of the switch.
@property(nonatomic, assign, getter=isOn) BOOL on;

@end

// StorageSwitchCell implements a UICollectionViewCell subclass containing a
// text label, a switch and a tooltip button.
@interface StorageSwitchCell : MDCCollectionViewCell

// Label displaying a standard text.
@property(nonatomic, readonly, strong) UILabel* textLabel;

// The tooltip button. Clients own and manage the tooltip they present and can
// use the button frame as the anchor point. Otherwise, this button is a no-op.
@property(nonatomic, readonly, strong) UIButton* tooltipButton;

// The switch view.
@property(nonatomic, readonly, strong) UISwitch* switchView;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_CELLS_STORAGE_SWITCH_ITEM_H_
