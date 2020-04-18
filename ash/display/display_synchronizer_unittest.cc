// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ui/aura/test/mus/test_window_manager_client.h"
#include "ui/aura/test/mus/test_window_tree_client_setup.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/insets.h"

namespace ash {

using DisplaySynchronizerTest = AshTestBase;

TEST_F(DisplaySynchronizerTest, ChangingWorkAreaNotifesServer) {
  aura::TestWindowManagerClient* test_window_manager_client =
      ash_test_helper()
          ->window_tree_client_setup()
          ->test_window_manager_client();
  const size_t initial_count =
      test_window_manager_client->GetChangeCountForType(
          aura::WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION);
  gfx::Insets work_area_insets =
      display::Screen::GetScreen()->GetPrimaryDisplay().GetWorkAreaInsets();
  work_area_insets += gfx::Insets(1);
  Shell::Get()->SetDisplayWorkAreaInsets(Shell::GetPrimaryRootWindow(),
                                         work_area_insets);
  EXPECT_EQ(
      initial_count + 1,
      test_window_manager_client->GetChangeCountForType(
          aura::WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION));
}

TEST_F(DisplaySynchronizerTest, AddingDisplayNotifies) {
  aura::TestWindowManagerClient* test_window_manager_client =
      ash_test_helper()
          ->window_tree_client_setup()
          ->test_window_manager_client();
  const size_t initial_count =
      test_window_manager_client->GetChangeCountForType(
          aura::WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION);
  // Ideally we would use an id set by UpdateDisplay(), but we need to set
  // the internal display id before calling UpdateDipslay().
  const int64_t internal_display_id = 10;
  // AshTestBase resets this for us.
  display::Display::SetInternalDisplayId(internal_display_id);
  UpdateDisplay("400x400,400x400");
  // Multiple calls may be sent, so we only check the count changes.
  EXPECT_NE(
      initial_count,
      test_window_manager_client->GetChangeCountForType(
          aura::WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION));
  ASSERT_TRUE(display::Display::HasInternalDisplay());
  EXPECT_EQ(internal_display_id,
            test_window_manager_client->last_internal_display_id());
}

TEST_F(DisplaySynchronizerTest,
       FrameDecorationsInstalledBeforeDisplayConfiguration) {
  aura::TestWindowManagerClient* test_window_manager_client =
      ash_test_helper()
          ->window_tree_client_setup()
          ->test_window_manager_client();
  ASSERT_NE(
      0u, test_window_manager_client->GetChangeCountForType(
              aura::WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION));
  ASSERT_NE(0u,
            test_window_manager_client->GetChangeCountForType(
                aura::WindowManagerClientChangeType::SET_FRAME_DECORATIONS));
  const size_t frame_decoration_change_index =
      test_window_manager_client->IndexOfFirstChangeOfType(
          aura::WindowManagerClientChangeType::SET_FRAME_DECORATIONS);
  const size_t display_config_change_index =
      test_window_manager_client->IndexOfFirstChangeOfType(
          aura::WindowManagerClientChangeType::SET_DISPLAY_CONFIGURATION);
  // Frame decorations must be installed before the display configuration is
  // set, otherwise clients are notified of bogus window manager frame values.
  EXPECT_LT(frame_decoration_change_index, display_config_change_index);
}

}  // namespace ash
