// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/page_range.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PageRangeTest, RangeMerge) {
  printing::PageRanges ranges;
  printing::PageRange range;
  range.from = 1;
  range.to = 3;
  ranges.push_back(range);
  range.from = 10;
  range.to = 12;
  ranges.push_back(range);
  range.from = 2;
  range.to = 5;
  ranges.push_back(range);
  std::vector<int> pages(printing::PageRange::GetPages(ranges));
  ASSERT_EQ(8U, pages.size());
  EXPECT_EQ(1, pages[0]);
  EXPECT_EQ(2, pages[1]);
  EXPECT_EQ(3, pages[2]);
  EXPECT_EQ(4, pages[3]);
  EXPECT_EQ(5, pages[4]);
  EXPECT_EQ(10, pages[5]);
  EXPECT_EQ(11, pages[6]);
  EXPECT_EQ(12, pages[7]);
}

TEST(PageRangeTest, Empty) {
  printing::PageRanges ranges;
  std::vector<int> pages(printing::PageRange::GetPages(ranges));
  EXPECT_TRUE(pages.empty());
}

TEST(PageRangeTest, Huge) {
  printing::PageRanges ranges;
  printing::PageRange range;
  range.from = 1;
  range.to = 2000000000;
  ranges.push_back(range);
  std::vector<int> pages(printing::PageRange::GetPages(ranges));
  EXPECT_FALSE(pages.empty());
}
