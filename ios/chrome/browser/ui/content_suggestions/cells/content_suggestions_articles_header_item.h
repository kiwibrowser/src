// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_ARTICLES_HEADER_ITEM_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_ARTICLES_HEADER_ITEM_H_

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

@class ContentSuggestionsArticlesHeaderCell;

// Delegate for the cell.
@protocol ContentSuggestionsArticlesHeaderCellDelegate

// Notifies the delegate that the button of the |cell| has been tapped.
- (void)cellButtonTapped:(nonnull ContentSuggestionsArticlesHeaderCell*)cell;

@end

// Item for a ArticlesHeader of a Content Suggestions section.  Displays a
// header title and a button to toggle the collapsed/expanded state.
@interface ContentSuggestionsArticlesHeaderItem
    : CollectionViewItem<ContentSuggestionsArticlesHeaderCellDelegate>

@property(nonatomic, assign) BOOL expanded;

// Initializes an ArticlesHeader with a button aligned to the trailing edge,
// with a |title| and a |callback| run when the cell button is tapped.
- (nullable instancetype)initWithType:(NSInteger)type
                                title:(nonnull NSString*)title
                             callback:(void (^_Nullable)())callback
    NS_DESIGNATED_INITIALIZER;

- (nullable instancetype)initWithType:(NSInteger)type NS_UNAVAILABLE;

@end

// Corresponding cell for a Content Suggestions' section ArticlesHeader.
@interface ContentSuggestionsArticlesHeaderCell : MDCCollectionViewCell

// Label used to indicate expanded state and trigger callback.
@property(nonatomic, readonly, strong, nonnull) UIButton* button;

// Label for this header items.
@property(nonatomic, strong, nonnull) UILabel* label;

@property(nonatomic, weak, nullable)
    id<ContentSuggestionsArticlesHeaderCellDelegate>
        delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_ARTICLES_HEADER_ITEM_H_
