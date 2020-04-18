// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_ENTRY_ITEM_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_ENTRY_ITEM_DELEGATE_H_

@class LegacyHistoryEntryItem;

// Delegate for HistoryEntryItem. Handles actions invoked as custom
// accessibility actions.
@protocol HistoryEntryItemDelegate
// Called when custom accessibility action to delete the entry is invoked.
- (void)historyEntryItemDidRequestDelete:(LegacyHistoryEntryItem*)item;
// Called when custom accessibility action to open the entry in a new tab is
// invoked.
- (void)historyEntryItemDidRequestOpenInNewTab:(LegacyHistoryEntryItem*)item;
// Called when custom accessibility action to open the entry in a new incognito
// tab is invoked.
- (void)historyEntryItemDidRequestOpenInNewIncognitoTab:
    (LegacyHistoryEntryItem*)item;
// Called when custom accessibility action to copy the entry's URL is invoked.
- (void)historyEntryItemDidRequestCopy:(LegacyHistoryEntryItem*)item;
// Called when the view associated with the HistoryEntryItem should be updated.
- (void)historyEntryItemShouldUpdateView:(LegacyHistoryEntryItem*)item;
@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_ENTRY_ITEM_DELEGATE_H_
