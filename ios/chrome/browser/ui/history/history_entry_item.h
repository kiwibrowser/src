// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_ENTRY_ITEM_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_HISTORY_ENTRY_ITEM_H_

#import "ios/chrome/browser/ui/history/history_entry_item_interface.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"

// Item that display a History entry. A history entry contains the title of the
// website, the URL and a timestamp of a previously visited website.
@interface HistoryEntryItem : TableViewItem<HistoryEntryItemInterface>

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_LEGACY_HISTORY_ENTRY_ITEM_H_
