// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"

namespace ash {

using ScreenAshTest = AshTestBase;

// Tests that ScreenAsh::GetWindowAtScreenPoint() returns the correct window on
// the correct display.
TEST_F(ScreenAshTest, TestGetWindowAtScreenPoint) {
  UpdateDisplay("200x200,400x400");

  aura::test::TestWindowDelegate delegate;
  std::unique_ptr<aura::Window> win1(CreateTestWindowInShellWithDelegate(
      &delegate, 0, gfx::Rect(0, 0, 200, 200)));

  std::unique_ptr<aura::Window> win2(CreateTestWindowInShellWithDelegate(
      &delegate, 1, gfx::Rect(200, 200, 100, 100)));

  ASSERT_NE(win1->GetRootWindow(), win2->GetRootWindow());

  EXPECT_EQ(win1.get(), display::Screen::GetScreen()->GetWindowAtScreenPoint(
                            gfx::Point(50, 60)));
  EXPECT_EQ(win2.get(), display::Screen::GetScreen()->GetWindowAtScreenPoint(
                            gfx::Point(250, 260)));
}

}  // namespace ash
