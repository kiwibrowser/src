// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_LAYOUT_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_LAYOUT_H_

#import "ios/third_party/material_components_ios/src/components/Collections/src/MDCCollectionViewFlowLayout.h"

// Layout used for ContentSuggestions. It makes sure the collection is high
// enough to be scrolled up to the point the fake omnibox is hidden. For size
// classes other than RegularXRegular, this layout makes sure the fake omnibox
// is pinned to the top of the collection.
@interface ContentSuggestionsLayout : MDCCollectionViewFlowLayout

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_LAYOUT_H_
