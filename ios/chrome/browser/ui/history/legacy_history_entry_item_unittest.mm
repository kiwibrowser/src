// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/legacy_history_entry_item.h"

#include "base/i18n/time_formatting.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using history::BrowsingHistoryService;

namespace {
const char kTestUrl[] = "http://test/";
const char kTestUrl2[] = "http://test2/";
const char kTestTitle[] = "Test";
}

LegacyHistoryEntryItem* GetHistoryEntryItem(const GURL& url,
                                            const char title[],
                                            base::Time timestamp) {
  BrowsingHistoryService::HistoryEntry entry(
      BrowsingHistoryService::HistoryEntry::LOCAL_ENTRY, GURL(url),
      base::UTF8ToUTF16(title), timestamp, "", false, base::string16(), false);
  LegacyHistoryEntryItem* item =
      [[LegacyHistoryEntryItem alloc] initWithType:0
                                      historyEntry:entry
                                      browserState:nil
                                          delegate:nil];
  return item;
}

using LegacyHistoryEntryItemTest = PlatformTest;

// Tests that -[HistoryEntryItem configureCell:] sets the cell's textLabel text
// to the item title, the detailTextLabel text to the URL, and the timeLabel
// text to the timestamp.
TEST_F(LegacyHistoryEntryItemTest, ConfigureCell) {
  base::Time timestamp = base::Time::Now();
  LegacyHistoryEntryItem* item =
      GetHistoryEntryItem(GURL(kTestUrl), kTestTitle, timestamp);

  LegacyHistoryEntryCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[LegacyHistoryEntryCell class]]);
  [item configureCell:cell];
  EXPECT_NSEQ(base::SysUTF8ToNSString(kTestTitle), cell.textLabel.text);
  EXPECT_NSEQ(base::SysUTF8ToNSString(kTestUrl), cell.detailTextLabel.text);
  EXPECT_NSEQ(base::SysUTF16ToNSString(base::TimeFormatTimeOfDay(timestamp)),
              cell.timeLabel.text);
}

// Tests that -[HistoryItem isEqualToHistoryItem:] returns YES if the two items
// have the same URL and timestamp, and NO otherwise.
TEST_F(LegacyHistoryEntryItemTest, IsEqual) {
  base::Time timestamp = base::Time::Now();
  base::Time timestamp2 = timestamp - base::TimeDelta::FromMinutes(1);
  LegacyHistoryEntryItem* history_entry =
      GetHistoryEntryItem(GURL(kTestUrl), kTestTitle, timestamp);
  LegacyHistoryEntryItem* same_entry =
      GetHistoryEntryItem(GURL(kTestUrl), kTestTitle, timestamp);

  LegacyHistoryEntryItem* different_time_entry =
      GetHistoryEntryItem(GURL(kTestUrl), kTestTitle, timestamp2);
  LegacyHistoryEntryItem* different_url_entry =
      GetHistoryEntryItem(GURL(kTestUrl2), kTestTitle, timestamp);

  EXPECT_TRUE([history_entry isEqual:same_entry]);
  EXPECT_FALSE([history_entry isEqual:different_time_entry]);
  EXPECT_FALSE([history_entry isEqual:different_url_entry]);
}
