// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRIES_STATUS_ITEM_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRIES_STATUS_ITEM_H_

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"

@class LabelLinkController;
@protocol HistoryEntriesStatusItemDelegate;

// Model item for HistoryEntriesStatusCell. Manages links added to the cell.
@interface LegacyHistoryEntriesStatusItem : CollectionViewItem
// YES if messages should be hidden.
@property(nonatomic, assign, getter=isHidden) BOOL hidden;
// Delegate for HistoryEntriesStatusItem. Is notified when a link is pressed.
@property(nonatomic, weak) id<HistoryEntriesStatusItemDelegate> delegate;
@end

// Cell for displaying status for history entry. Provides information on whether
// local or synced entries or displays, and how to access other forms of
// browsing history, if applicable.
@interface LegacyHistoryEntriesStatusCell : CollectionViewFooterCell
@end

@interface LegacyHistoryEntriesStatusCell (Testing)
// Link controller for entries status message.
@property(nonatomic, retain, readonly) LabelLinkController* labelLinkController;
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRIES_STATUS_ITEM_H_
