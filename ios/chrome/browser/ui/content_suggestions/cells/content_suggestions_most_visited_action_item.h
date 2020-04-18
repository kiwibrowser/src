// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_MOST_VISITED_ACTION_ITEM_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_MOST_VISITED_ACTION_ITEM_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/content_suggestions/cells/suggested_content.h"

// Enum defining the actions for a ContentSuggestionsMostVisitedActionItem.
typedef NS_ENUM(NSInteger, ContentSuggestionsMostVisitedAction) {
  ContentSuggestionsMostVisitedActionBookmark,
  ContentSuggestionsMostVisitedActionReadingList,
  ContentSuggestionsMostVisitedActionRecentTabs,
  ContentSuggestionsMostVisitedActionHistory,
};

// Item containing a most visited action button. These buttons belong to the
// collection section as most visited items, but have static placement (the last
// four) and cannot be removed.
@interface ContentSuggestionsMostVisitedActionItem
    : CollectionViewItem<SuggestedContent>

- (nonnull instancetype)initWithAction:
    (ContentSuggestionsMostVisitedAction)action;

// Text for the title and the accessibility label of the cell.
@property(nonatomic, copy, nonnull) NSString* title;

// The action type for this button.
@property(nonatomic, assign) ContentSuggestionsMostVisitedAction action;

// Reading list count passed to the most visited cell.
@property(nonatomic, assign) NSInteger count;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_MOST_VISITED_ACTION_ITEM_H_
