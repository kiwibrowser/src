// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRY_ITEM_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRY_ITEM_H_

#include "components/history/core/browser/browsing_history_service.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/history/history_entry_item_interface.h"
#import "ios/third_party/material_components_ios/src/components/Collections/src/MaterialCollections.h"

namespace ios {
class ChromeBrowserState;
}  // namespace ios

@class FaviconView;
@protocol FaviconViewProviderDelegate;
@protocol HistoryEntryItemDelegate;

// Model object for the cell that displays a history entry.
@interface LegacyHistoryEntryItem
    : CollectionViewItem<HistoryEntryItemInterface>

// The |delegate| is notified when the favicon has loaded, and may be nil.
- (instancetype)initWithType:(NSInteger)type
                historyEntry:
                    (const history::BrowsingHistoryService::HistoryEntry&)entry
                browserState:(ios::ChromeBrowserState*)browserState
                    delegate:(id<HistoryEntryItemDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithType:(NSInteger)type NS_UNAVAILABLE;

@end

// Cell that renders a history entry.
@interface LegacyHistoryEntryCell : MDCCollectionViewCell

// View for displaying the favicon for the history entry.
@property(nonatomic, strong) UIView* faviconViewContainer;
// Text label for history entry title.
@property(nonatomic, readonly, strong) UILabel* textLabel;
// Text label for history entry URL.
@property(nonatomic, readonly, strong) UILabel* detailTextLabel;
// Text label for history entry timestamp.
@property(nonatomic, readonly, strong) UILabel* timeLabel;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRY_ITEM_H_
