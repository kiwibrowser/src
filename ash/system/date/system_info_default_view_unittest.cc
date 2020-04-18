// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/date/system_info_default_view.h"
#include "ash/system/tray/tray_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view.h"

namespace ash {

class SystemInfoDefaultViewTest : public testing::Test {
 public:
  SystemInfoDefaultViewTest() = default;

 protected:
  // Wrapper calls for SystemInfoDefaultView internals.
  int CalculateDateViewWidth(int preferred_width) {
    return SystemInfoDefaultView::CalculateDateViewWidth(preferred_width);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemInfoDefaultViewTest);
};

TEST_F(SystemInfoDefaultViewTest, PreferredWidthSmallerThanButtonWidth) {
  const int kPreferredWidth = 10;
  EXPECT_LT(kPreferredWidth, kMenuButtonSize);
  const int effective_width = CalculateDateViewWidth(kPreferredWidth);

  EXPECT_EQ(effective_width,
            kMenuButtonSize + kSeparatorWidth + kMenuButtonSize);
}

TEST_F(SystemInfoDefaultViewTest, PreferredWidthGreaterThanOneButtonWidth) {
  const int kPreferredWidth = kMenuButtonSize + 1;
  const int effective_width = CalculateDateViewWidth(kPreferredWidth);

  EXPECT_EQ(effective_width,
            kMenuButtonSize + kSeparatorWidth + kMenuButtonSize);
}

TEST_F(SystemInfoDefaultViewTest, PreferredWidthEqualToTwoButtonWidths) {
  const int kPreferredWidth =
      kMenuButtonSize + kSeparatorWidth + kMenuButtonSize;
  const int effective_width = CalculateDateViewWidth(kPreferredWidth);

  EXPECT_EQ(effective_width, kPreferredWidth);
}

TEST_F(SystemInfoDefaultViewTest, PreferredWidthGreaterThanTwoButtonWidths) {
  const int kPreferredWidth =
      kMenuButtonSize + (kSeparatorWidth + kMenuButtonSize) + 1;
  const int effective_width = CalculateDateViewWidth(kPreferredWidth);

  EXPECT_EQ(effective_width,
            kMenuButtonSize + (kSeparatorWidth + kMenuButtonSize) * 2);
}

TEST_F(SystemInfoDefaultViewTest, PreferredWidthEqualToThreeButtonWidths) {
  const int kPreferredWidth =
      kMenuButtonSize + (kSeparatorWidth + kMenuButtonSize) * 2;
  const int effective_width = CalculateDateViewWidth(kPreferredWidth);

  EXPECT_EQ(effective_width, kPreferredWidth);
}

TEST_F(SystemInfoDefaultViewTest, PreferredWidthGreaterThanThreeButtonWidths) {
  const int kPreferredWidth =
      kMenuButtonSize + (kSeparatorWidth + kMenuButtonSize) * 2 + 1;
  const int effective_width = CalculateDateViewWidth(kPreferredWidth);

  EXPECT_EQ(effective_width,
            kMenuButtonSize + (kSeparatorWidth + kMenuButtonSize) * 2);
}

}  // namespace ash
