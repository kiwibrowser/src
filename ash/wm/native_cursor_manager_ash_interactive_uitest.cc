// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/native_cursor_manager_ash.h"

#include "ash/shell.h"
#include "ash/test/ash_interactive_ui_test_base.h"
#include "ash/wm/cursor_manager_test_api.h"
#include "base/run_loop.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/test/ui_controls.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"

namespace ash {

using NativeCursorManagerAshTest = AshInteractiveUITestBase;

namespace {

display::ManagedDisplayInfo CreateDisplayInfo(int64_t id,
                                              const gfx::Rect& bounds,
                                              float device_scale_factor) {
  display::ManagedDisplayInfo info(id, "", false);
  info.SetBounds(bounds);
  info.set_device_scale_factor(device_scale_factor);
  return info;
}

void MoveMouseSync(aura::Window* window, int x, int y) {
  // Send and wait for a key event to make sure that mouse
  // events are fully processed.
  base::RunLoop loop;
  ui_controls::SendKeyPressNotifyWhenDone(window, ui::VKEY_SPACE, false, false,
                                          false, false, loop.QuitClosure());
  loop.Run();
}

}  // namespace

// Disabled on non-X11 before X11 was deprecated.
TEST_F(NativeCursorManagerAshTest, DISABLED_CursorChangeOnEnterNotify) {
  ::wm::CursorManager* cursor_manager = Shell::Get()->cursor_manager();
  CursorManagerTestApi test_api(cursor_manager);

  display::ManagedDisplayInfo display_info1 =
      CreateDisplayInfo(10, gfx::Rect(0, 0, 500, 300), 1.0f);
  display::ManagedDisplayInfo display_info2 =
      CreateDisplayInfo(20, gfx::Rect(500, 0, 500, 300), 2.0f);
  std::vector<display::ManagedDisplayInfo> display_info_list;
  display_info_list.push_back(display_info1);
  display_info_list.push_back(display_info2);
  display_manager()->OnNativeDisplaysChanged(display_info_list);

  MoveMouseSync(Shell::GetAllRootWindows()[0], 10, 10);
  EXPECT_EQ(1.0f, test_api.GetCurrentCursor().device_scale_factor());

  MoveMouseSync(Shell::GetAllRootWindows()[0], 600, 10);
  EXPECT_EQ(2.0f, test_api.GetCurrentCursor().device_scale_factor());
}

}  // namespace ash
