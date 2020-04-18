// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_commands.h"

#include <memory>

#include "ash/accelerators/accelerator_commands.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"

// Note: The unit tests for |ToggleMaximized()| and
// |ToggleFullscreen()| are in
// chrome/browser/ui/ash/accelerator_commands_browsertests.cc.
// because they depends on chrome implementation of
// |ash::wm::WindowStateDelegate|.

namespace ash {
namespace accelerators {

using AcceleratorCommandsTest = AshTestBase;

TEST_F(AcceleratorCommandsTest, ToggleMinimized) {
  std::unique_ptr<aura::Window> window1(
      CreateTestWindowInShellWithBounds(gfx::Rect(5, 5, 20, 20)));
  std::unique_ptr<aura::Window> window2(
      CreateTestWindowInShellWithBounds(gfx::Rect(5, 5, 20, 20)));
  wm::WindowState* window_state1 = wm::GetWindowState(window1.get());
  wm::WindowState* window_state2 = wm::GetWindowState(window2.get());
  window_state1->Activate();
  window_state2->Activate();

  ToggleMinimized();
  EXPECT_TRUE(window_state2->IsMinimized());
  EXPECT_FALSE(window_state2->IsNormalStateType());
  EXPECT_TRUE(window_state1->IsActive());

  ToggleMinimized();
  EXPECT_TRUE(window_state1->IsMinimized());
  EXPECT_FALSE(window_state1->IsNormalStateType());
  EXPECT_FALSE(window_state1->IsActive());

  // Toggling minimize when there are no active windows should unminimize and
  // activate the last active window.
  ToggleMinimized();
  EXPECT_FALSE(window_state1->IsMinimized());
  EXPECT_TRUE(window_state1->IsNormalStateType());
  EXPECT_TRUE(window_state1->IsActive());
}

TEST_F(AcceleratorCommandsTest, Unpin) {
  std::unique_ptr<aura::Window> window1(
      CreateTestWindowInShellWithBounds(gfx::Rect(5, 5, 20, 20)));
  wm::WindowState* window_state1 = wm::GetWindowState(window1.get());
  window_state1->Activate();

  wm::PinWindow(window1.get(), /* trusted */ false);
  EXPECT_TRUE(window_state1->IsPinned());

  UnpinWindow();
  EXPECT_FALSE(window_state1->IsPinned());
}

}  // namespace accelerators
}  // namespace ash
