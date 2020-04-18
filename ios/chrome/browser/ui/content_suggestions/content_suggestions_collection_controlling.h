// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_CONTROLLING_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_CONTROLLING_H_

#import <UIKit/UIKit.h>

@protocol ContentSuggestionsHeaderSynchronizing;

// Controller for the ContentSuggestions collection.
@protocol ContentSuggestionsCollectionControlling

// |YES| if the collection scrollView is scrolled all the way to the top. Used
// to lock this position in place on various frame changes.
@property(nonatomic, assign, getter=isScrolledToTop) BOOL scrolledToTop;

// Synchronizer for the collection controller, allowing it to synchronize with
// its header.
@property(nonatomic, weak) id<ContentSuggestionsHeaderSynchronizing>
    headerSynchronizer;

- (UICollectionView*)collectionView;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COLLECTION_CONTROLLING_H_
