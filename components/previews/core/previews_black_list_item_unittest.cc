// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_black_list_item.h"

#include <memory>

#include "base/optional.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using PreviewsBlackListItemTest = testing::Test;

}  // namespace

namespace previews {

TEST_F(PreviewsBlackListItemTest, BlackListState) {
  const int history = 4;
  const int threshold = 2;
  const base::TimeDelta max_blacklist_duration =
      base::TimeDelta::FromSeconds(30);
  const base::Time now = base::Time::UnixEpoch();
  const base::TimeDelta delay_between_entries = base::TimeDelta::FromSeconds(1);
  const base::Time later =
      now + max_blacklist_duration + (delay_between_entries * 3);

  PreviewsBlackListItem black_list_item(history, threshold,
                                        max_blacklist_duration);

  // Empty black list item should report that the host is allowed.
  EXPECT_FALSE(black_list_item.IsBlackListed(now));
  EXPECT_FALSE(black_list_item.IsBlackListed(later));

  EXPECT_FALSE(black_list_item.most_recent_opt_out_time());
  black_list_item.AddPreviewNavigation(false, now);
  EXPECT_FALSE(black_list_item.most_recent_opt_out_time());

  black_list_item.AddPreviewNavigation(true, now);
  EXPECT_TRUE(black_list_item.most_recent_opt_out_time());
  EXPECT_EQ(now, black_list_item.most_recent_opt_out_time().value());
  // Black list item of size less that |threshold| should report that the host
  // is allowed.
  EXPECT_FALSE(black_list_item.IsBlackListed(now));
  EXPECT_FALSE(black_list_item.IsBlackListed(later));

  black_list_item.AddPreviewNavigation(true, now + delay_between_entries);
  // Black list item with |threshold| fresh entries should report the host as
  // disallowed.
  EXPECT_TRUE(black_list_item.IsBlackListed(now));
  // Black list item with only entries from longer than |duration| ago should
  // report the host is allowed.
  EXPECT_FALSE(black_list_item.IsBlackListed(later));
  black_list_item.AddPreviewNavigation(true,
                                       later - (delay_between_entries * 2));
  // Black list item with a fresh opt out and total number of opt outs larger
  // than |threshold| should report the host is disallowed.
  EXPECT_TRUE(black_list_item.IsBlackListed(later));

  // The black list item should maintain entries based on time, so adding
  // |history| entries should not push out newer entries.
  black_list_item.AddPreviewNavigation(true, later - delay_between_entries * 2);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 3);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 3);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 3);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 3);
  EXPECT_TRUE(black_list_item.IsBlackListed(later));

  // The black list item should maintain entries based on time, so adding
  // |history| newer entries should push out older entries.
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 1);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 1);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 1);
  black_list_item.AddPreviewNavigation(false,
                                       later - delay_between_entries * 1);
  EXPECT_FALSE(black_list_item.IsBlackListed(later));
}

}  // namespace previews
