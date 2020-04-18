// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_GESTURE_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_GESTURE_COMMANDS_H_

#import <UIKit/UIKit.h>

@class ContentSuggestionsItem;
@class ContentSuggestionsMostVisitedItem;

// Command protocol for the interactions based on a gesture, handling the
// callbacks from the alerts and the accessibility custom actions.
@protocol ContentSuggestionsGestureCommands

// Opens the URL corresponding to the |item| in a new tab, |incognito| or not.
// The item has to be a suggestion item.
- (void)openNewTabWithSuggestionsItem:(nonnull ContentSuggestionsItem*)item
                            incognito:(BOOL)incognito;

// Adds the |item| to the reading list. The item has to be a suggestion item.
- (void)addItemToReadingList:(nonnull ContentSuggestionsItem*)item;

// Dismiss the |item| at |indexPath|. The item has to be a suggestion item.
// If |indexPath| is nil, the commands handler will find the index path
// associated with the |item|.
- (void)dismissSuggestion:(nonnull ContentSuggestionsItem*)item
              atIndexPath:(nullable NSIndexPath*)indexPath;

// Open the URL corresponding to the |item| in a new tab, |incognito| or not.
// The item has to be a Most Visited item.
- (void)openNewTabWithMostVisitedItem:
            (nonnull ContentSuggestionsMostVisitedItem*)item
                            incognito:(BOOL)incognito
                              atIndex:(NSInteger)mostVisitedIndex;

// Open the URL corresponding to the |item| in a new tab, |incognito| or not.
// The index of the item will be find by the  command handler. The item has to
// be a Most Visited item.
- (void)openNewTabWithMostVisitedItem:
            (nonnull ContentSuggestionsMostVisitedItem*)item
                            incognito:(BOOL)incognito;

// Removes the most visited |item|.
- (void)removeMostVisited:(nonnull ContentSuggestionsMostVisitedItem*)item;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CELLS_CONTENT_SUGGESTIONS_GESTURE_COMMANDS_H_
