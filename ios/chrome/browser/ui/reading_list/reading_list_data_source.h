// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_DATA_SOURCE_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_DATA_SOURCE_H_

#include <memory>

@class CollectionViewItem;
@protocol ReadingListDataSink;
@protocol ReadingListCollectionViewItemAccessibilityDelegate;

// Data Source for the Reading List UI, providing the data sink with the data to
// be displayed. Handle the interactions with the model.
@protocol ReadingListDataSource

// The data sink associated with this data source.
@property(nonatomic, weak, nullable) id<ReadingListDataSink> dataSink;
// Whether the data source is ready to be used.
- (BOOL)ready;
// Whether the data source has some elements.
- (BOOL)hasElements;
// Whether the data source has some read elements.
- (BOOL)hasRead;
// Whether the entry corresponding to the |item| is read.
- (BOOL)isEntryRead:(nonnull CollectionViewItem*)item;

// Mark all entries as seen and stop sending updates to the data sink.
- (void)dataSinkWillBeDismissed;

// Set the read status of the entry associated with |item|.
- (void)setReadStatus:(BOOL)read forItem:(nonnull CollectionViewItem*)item;

// Removes the entry associated with |item| and logs the deletion.
- (void)removeEntryFromItem:(nonnull CollectionViewItem*)item;

// Fills the |readArray| and |unreadArray| with the corresponding items from the
// model. The items are sorted most recent first.
- (void)fillReadItems:(nullable NSMutableArray<CollectionViewItem*>*)readArray
          unreadItems:(nullable NSMutableArray<CollectionViewItem*>*)unreadArray
         withDelegate:
             (nullable id<ReadingListCollectionViewItemAccessibilityDelegate>)
                 delegate;

// Fetches the |faviconURL| of this |item|, notifies the data sink when
// receiving the favicon.
- (void)fetchFaviconForItem:(nonnull CollectionViewItem*)item;

// Prepares the data source for batch updates. The UI is not notified for the
// updates happenning between |-beginBatchUpdates| and |-endBatchUpdates|.
- (void)beginBatchUpdates;
// Notifies the data source that the batch updates are over. After calling this
// function the UI is notified when the model changes.
- (void)endBatchUpdates;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_DATA_SOURCE_H_
