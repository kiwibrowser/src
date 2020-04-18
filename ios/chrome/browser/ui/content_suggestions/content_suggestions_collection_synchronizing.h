// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_SYNCHRONIZING_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_SYNCHRONIZING_H_

#import "base/ios/block_types.h"

// Synchronization protocol used by the ContentSuggestions header controller to
// synchronize with the ContentSuggestions collection.
@protocol ContentSuggestionsCollectionSynchronizing

// Moves the tiles down, by setting the content offset of the collection to 0.
- (void)shiftTilesDown;
// Moves the tiles up by pinning the omnibox to the top. Completion called only
// if scrolled to top.
- (void)shiftTilesUpWithCompletionBlock:(ProceduralBlock)completionBlock;
// Notifies the collection that its layout has changed and should be
// invalidated.
- (void)invalidateLayout;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_SYNCHRONIZING_H_
