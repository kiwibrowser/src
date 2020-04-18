// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/page_number.h"
#include "printing/print_settings.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PageNumberTest, Count) {
  printing::PrintSettings settings;
  printing::PageNumber page;
  EXPECT_EQ(printing::PageNumber::npos(), page);
  page.Init(settings, 3);
  EXPECT_EQ(0, page.ToInt());
  EXPECT_NE(printing::PageNumber::npos(), page);
  ++page;
  EXPECT_EQ(1, page.ToInt());
  EXPECT_NE(printing::PageNumber::npos(), page);

  printing::PageNumber page_copy(page);
  EXPECT_EQ(1, page_copy.ToInt());
  EXPECT_EQ(1, page.ToInt());
  ++page;
  EXPECT_EQ(1, page_copy.ToInt());
  EXPECT_EQ(2, page.ToInt());
  ++page;
  EXPECT_EQ(printing::PageNumber::npos(), page);
  ++page;
  EXPECT_EQ(printing::PageNumber::npos(), page);
}
