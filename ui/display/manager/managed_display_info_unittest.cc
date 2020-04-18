// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/managed_display_info.h"

#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "ui/display/manager/touch_device_manager.h"
#endif

namespace display {
namespace {

std::string GetModeSizeInDIP(const gfx::Size& size,
                             float device_scale_factor,
                             float ui_scale,
                             bool is_internal) {
  ManagedDisplayMode mode(size, 0.0 /* refresh_rate */, false /* interlaced */,
                          false /* native */, ui_scale, device_scale_factor);
  return mode.GetSizeInDIP(is_internal).ToString();
}

}  // namespace

typedef testing::Test DisplayInfoTest;

TEST_F(DisplayInfoTest, CreateFromSpec) {
  ManagedDisplayInfo info =
      ManagedDisplayInfo::CreateFromSpecWithID("200x100", 10);
  EXPECT_EQ(10, info.id());
  EXPECT_EQ("0,0 200x100", info.bounds_in_native().ToString());
  EXPECT_EQ("200x100", info.size_in_pixel().ToString());
  EXPECT_EQ(Display::ROTATE_0, info.GetActiveRotation());
  EXPECT_EQ("0,0,0,0", info.overscan_insets_in_dip().ToString());
  EXPECT_EQ(1.0f, info.configured_ui_scale());

  info = ManagedDisplayInfo::CreateFromSpecWithID("10+20-300x400*2/o", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_native().ToString());
  EXPECT_EQ("288x380", info.size_in_pixel().ToString());
  EXPECT_EQ(Display::ROTATE_0, info.GetActiveRotation());
  EXPECT_EQ("5,3,5,3", info.overscan_insets_in_dip().ToString());

  info = ManagedDisplayInfo::CreateFromSpecWithID("10+20-300x400*2/ob", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_native().ToString());
  EXPECT_EQ("288x380", info.size_in_pixel().ToString());
  EXPECT_EQ(Display::ROTATE_0, info.GetActiveRotation());
  EXPECT_EQ("5,3,5,3", info.overscan_insets_in_dip().ToString());

  info = ManagedDisplayInfo::CreateFromSpecWithID("10+20-300x400*2/or", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_native().ToString());
  EXPECT_EQ("380x288", info.size_in_pixel().ToString());
  EXPECT_EQ(Display::ROTATE_90, info.GetActiveRotation());
  // TODO(oshima): This should be rotated too. Fix this.
  EXPECT_EQ("5,3,5,3", info.overscan_insets_in_dip().ToString());

  info = ManagedDisplayInfo::CreateFromSpecWithID("10+20-300x400*2/l@1.5", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_native().ToString());
  EXPECT_EQ(Display::ROTATE_270, info.GetActiveRotation());
  EXPECT_EQ(1.5f, info.configured_ui_scale());

  info = ManagedDisplayInfo::CreateFromSpecWithID(
      "200x200#300x200|200x200%59.9|100x100%60|150x100*2|150x150*1.25%30", 10);

  EXPECT_EQ("0,0 200x200", info.bounds_in_native().ToString());
  EXPECT_EQ(5u, info.display_modes().size());
  // Modes are sorted in DIP for external display.
  EXPECT_EQ("150x100", info.display_modes()[0].size().ToString());
  EXPECT_EQ("100x100", info.display_modes()[1].size().ToString());
  EXPECT_EQ("150x150", info.display_modes()[2].size().ToString());
  EXPECT_EQ("200x200", info.display_modes()[3].size().ToString());
  EXPECT_EQ("300x200", info.display_modes()[4].size().ToString());

  EXPECT_EQ(0.0f, info.display_modes()[0].refresh_rate());
  EXPECT_EQ(60.0f, info.display_modes()[1].refresh_rate());
  EXPECT_EQ(30.0f, info.display_modes()[2].refresh_rate());
  EXPECT_EQ(59.9f, info.display_modes()[3].refresh_rate());
  EXPECT_EQ(0.0f, info.display_modes()[4].refresh_rate());

  EXPECT_EQ(2.0f, info.display_modes()[0].device_scale_factor());
  EXPECT_EQ(1.0f, info.display_modes()[1].device_scale_factor());
  EXPECT_EQ(1.25f, info.display_modes()[2].device_scale_factor());
  EXPECT_EQ(1.0f, info.display_modes()[3].device_scale_factor());
  EXPECT_EQ(1.0f, info.display_modes()[4].device_scale_factor());

  EXPECT_FALSE(info.display_modes()[0].native());
  EXPECT_FALSE(info.display_modes()[1].native());
  EXPECT_FALSE(info.display_modes()[2].native());
  EXPECT_FALSE(info.display_modes()[3].native());
  EXPECT_TRUE(info.display_modes()[4].native());
}

TEST_F(DisplayInfoTest, ManagedDisplayModeGetSizeInDIPNormal) {
  gfx::Size size(1366, 768);
  EXPECT_EQ("1536x864", GetModeSizeInDIP(size, 1.0f, 1.125f, true));
  EXPECT_EQ("1366x768", GetModeSizeInDIP(size, 1.0f, 1.0f, true));
  EXPECT_EQ("1092x614", GetModeSizeInDIP(size, 1.0f, 0.8f, true));
  EXPECT_EQ("853x480", GetModeSizeInDIP(size, 1.0f, 0.625f, true));
  EXPECT_EQ("683x384", GetModeSizeInDIP(size, 1.0f, 0.5f, true));
}

TEST_F(DisplayInfoTest, ManagedDisplayModeGetSizeInDIPHiDPI) {
  gfx::Size size(2560, 1700);
  EXPECT_EQ("2560x1700", GetModeSizeInDIP(size, 2.0f, 2.0f, true));
  EXPECT_EQ("1920x1275", GetModeSizeInDIP(size, 2.0f, 1.5f, true));
  EXPECT_EQ("1600x1062", GetModeSizeInDIP(size, 2.0f, 1.25f, true));
  EXPECT_EQ("1440x956", GetModeSizeInDIP(size, 2.0f, 1.125f, true));
  EXPECT_EQ("1280x850", GetModeSizeInDIP(size, 2.0f, 1.0f, true));
  EXPECT_EQ("1024x680", GetModeSizeInDIP(size, 2.0f, 0.8f, true));
  EXPECT_EQ("800x531", GetModeSizeInDIP(size, 2.0f, 0.625f, true));
  EXPECT_EQ("640x425", GetModeSizeInDIP(size, 2.0f, 0.5f, true));
}

TEST_F(DisplayInfoTest, ManagedDisplayModeGetSizeInDIP125) {
  gfx::Size size(1920, 1080);
  EXPECT_EQ("2400x1350", GetModeSizeInDIP(size, 1.25f, 1.25f, true));
  EXPECT_EQ("1920x1080", GetModeSizeInDIP(size, 1.25f, 1.0f, true));
  EXPECT_EQ("1536x864", GetModeSizeInDIP(size, 1.25f, 0.8f, true));
  EXPECT_EQ("1200x675", GetModeSizeInDIP(size, 1.25f, 0.625f, true));
  EXPECT_EQ("960x540", GetModeSizeInDIP(size, 1.25f, 0.5f, true));
}

TEST_F(DisplayInfoTest, ManagedDisplayModeGetSizeForExternal4K) {
  gfx::Size size(3840, 2160);
  EXPECT_EQ("1920x1080", GetModeSizeInDIP(size, 2.0f, 1.0f, false));
  EXPECT_EQ("3072x1728", GetModeSizeInDIP(size, 1.25f, 1.0f, false));
  EXPECT_EQ("3840x2160", GetModeSizeInDIP(size, 1.0f, 1.0f, false));
}

}  // namespace display
