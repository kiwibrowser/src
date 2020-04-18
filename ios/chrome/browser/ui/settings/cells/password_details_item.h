// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_PASSWORD_DETAILS_ITEM_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_PASSWORD_DETAILS_ITEM_H_

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

// Item to display an element of the Password Details settings.
@interface PasswordDetailsItem : CollectionViewItem

// The text (a username, a password).
@property(nonatomic, copy) NSString* text;

// Whether to reveal the text or ●●●●●●. There are as many dots as the text is
// long.
@property(nonatomic, assign, getter=isShowingText) BOOL showingText;

@end

// Cell class associated to PasswordDetailsItem. The text label can span
// multiple lines.
@interface PasswordDetailsCell : MDCCollectionViewCell

// Label for the text.
@property(nonatomic, readonly, strong) UILabel* textLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_CELLS_PASSWORD_DETAILS_ITEM_H_
