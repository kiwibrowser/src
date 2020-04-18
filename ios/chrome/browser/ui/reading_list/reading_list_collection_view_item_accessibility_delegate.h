// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COLLECTION_VIEW_ITEM_ACCESSIBILITY_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COLLECTION_VIEW_ITEM_ACCESSIBILITY_DELEGATE_H_

@class CollectionViewItem;

@protocol ReadingListCollectionViewItemAccessibilityDelegate

// Returns whether the entry is read.
- (BOOL)isEntryRead:(CollectionViewItem*)entry;

- (void)deleteEntry:(CollectionViewItem*)entry;
- (void)openEntryInNewTab:(CollectionViewItem*)entry;
- (void)openEntryInNewIncognitoTab:(CollectionViewItem*)entry;
- (void)openEntryOffline:(CollectionViewItem*)entry;
- (void)markEntryRead:(CollectionViewItem*)entry;
- (void)markEntryUnread:(CollectionViewItem*)entry;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COLLECTION_VIEW_ITEM_ACCESSIBILITY_DELEGATE_H_
