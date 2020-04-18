// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/memory/singleton.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/ui/cocoa/accelerators_cocoa.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "ui/base/accelerators/platform_accelerator_cocoa.h"

TEST(AcceleratorsCocoaTest, GetAccelerator) {
  AcceleratorsCocoa* keymap = AcceleratorsCocoa::GetInstance();
  const ui::Accelerator* accelerator =
      keymap->GetAcceleratorForCommand(IDC_COPY);
  ASSERT_TRUE(accelerator);
  ASSERT_TRUE(accelerator->platform_accelerator());
  const ui::PlatformAcceleratorCocoa* platform_accelerator =
      static_cast<const ui::PlatformAcceleratorCocoa*>(
          accelerator->platform_accelerator());
  EXPECT_NSEQ(@"c", platform_accelerator->characters());
  EXPECT_EQ(NSCommandKeyMask, platform_accelerator->modifier_mask());
}

TEST(AcceleratorsCocoaTest, GetNullAccelerator) {
  AcceleratorsCocoa* keymap = AcceleratorsCocoa::GetInstance();
  const ui::Accelerator* accelerator =
      keymap->GetAcceleratorForCommand(314159265);
  EXPECT_FALSE(accelerator);
}
