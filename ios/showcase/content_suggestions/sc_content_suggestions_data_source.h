// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHOWCASE_CONTENT_SUGGESTIONS_SC_CONTENT_SUGGESTIONS_DATA_SOURCE_H_
#define IOS_SHOWCASE_CONTENT_SUGGESTIONS_SC_CONTENT_SUGGESTIONS_DATA_SOURCE_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_data_source.h"

// Fake DataSource for the ContentSuggestions.
@interface SCContentSuggestionsDataSource
    : NSObject<ContentSuggestionsDataSource>

// Title of the first suggested article.
+ (NSString*)titleFirstSuggestion;

// Title of the Reading List item.
+ (NSString*)titleReadingListItem;

@end

#endif  // IOS_SHOWCASE_CONTENT_SUGGESTIONS_SC_CONTENT_SUGGESTIONS_DATA_SOURCE_H_
