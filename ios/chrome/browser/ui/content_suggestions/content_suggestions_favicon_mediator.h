// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_FAVICON_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_FAVICON_MEDIATOR_H_

#import <UIKit/UIKit.h>

#include "components/ntp_tiles/ntp_tile.h"

namespace favicon {
class LargeIconService;
}
namespace ntp_snippets {
class Category;
class ContentSuggestionsService;
}

@protocol ContentSuggestionsDataSink;
@class ContentSuggestionsItem;
@class ContentSuggestionsMostVisitedItem;
@class FaviconAttributesProvider;
class LargeIconCache;

// Mediator handling the fetching of the favicon for all ContentSuggestions
// items.
@interface ContentSuggestionsFaviconMediator : NSObject

// Initializes the mediator with the |contentService| used to fetch the image of
// the suggested items and the |largeIconService| to fetch the favicon locally.
- (nullable instancetype)
initWithContentService:
    (nonnull ntp_snippets::ContentSuggestionsService*)contentService
      largeIconService:(nonnull favicon::LargeIconService*)largeIconService
        largeIconCache:(nullable LargeIconCache*)largeIconCache
    NS_DESIGNATED_INITIALIZER;

- (nullable instancetype)init NS_UNAVAILABLE;

// The data sink which should be notified of the changes in the items.
@property(nonatomic, weak, nullable) id<ContentSuggestionsDataSink> dataSink;

// FaviconAttributesProvider to fetch the favicon for the most visited tiles.
@property(nonatomic, nullable, strong, readonly)
    FaviconAttributesProvider* mostVisitedAttributesProvider;

// Sets the |mostVisitedData| used to log the impression of the tiles.
- (void)setMostVisitedDataForLogging:
    (const ntp_tiles::NTPTilesVector&)mostVisitedData;

// Fetches the favicon for this |item|.
- (void)fetchFaviconForMostVisited:
    (nonnull ContentSuggestionsMostVisitedItem*)item;

// Fetches the favicon attributes for this |item| living in |category|. Also
// fetches the favicon image from the ContentSuggestionsService.
- (void)fetchFaviconForSuggestions:(nonnull ContentSuggestionsItem*)item
                        inCategory:(ntp_snippets::Category)category;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_FAVICON_MEDIATOR_H_
