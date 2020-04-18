// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_FOOTER_ITEM_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_FOOTER_ITEM_H_

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

@class ContentSuggestionsFooterCell;

// Delegate for the cell.
@protocol ContentSuggestionsFooterCellDelegate

// Notifies the delegate that the button of the |cell| has been tapped.
- (void)cellButtonTapped:(nonnull ContentSuggestionsFooterCell*)cell;

@end

// Item for a footer of a Content Suggestions section.
@interface ContentSuggestionsFooterItem
    : CollectionViewItem<ContentSuggestionsFooterCellDelegate>

// Initialize a footer with a button taking all the space, with a |title| and a
// |callback| run when the cell button is tapped.
- (nullable instancetype)
initWithType:(NSInteger)type
       title:(nonnull NSString*)title
    callback:(void (^_Nullable)(ContentSuggestionsFooterItem* _Nonnull,
                                ContentSuggestionsFooterCell* _Nonnull))callback
    NS_DESIGNATED_INITIALIZER;

- (nullable instancetype)initWithType:(NSInteger)type NS_UNAVAILABLE;

// Whether the item is loading.
@property(nonatomic, assign, getter=isLoading) BOOL loading;

// Last cell configured by this item. This cell might not be currently
// associated with this item.
@property(nonatomic, weak, nullable)
    ContentSuggestionsFooterCell* configuredCell;

@end

// Corresponding cell for a Content Suggestions' section footer.
@interface ContentSuggestionsFooterCell : MDCCollectionViewCell

@property(nonatomic, readonly, strong, nonnull) UIButton* button;

@property(nonatomic, weak, nullable) id<ContentSuggestionsFooterCellDelegate>
    delegate;

// Whether the cell is loading. If it is loading a spinner is displayed and the
// button is hidden.
- (void)setLoading:(BOOL)loading;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_FOOTER_ITEM_H_
