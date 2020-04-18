// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/flag_warning/flag_warning_tray.h"

#include "ash/public/cpp/config.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/system/status_area_widget.h"
#include "ash/test/ash_test_base.h"

namespace ash {
namespace {

using FlagWarningTrayTest = AshTestBase;

TEST_F(FlagWarningTrayTest, Visibility) {
  // Flag warning tray is not currently used in non-MASH environments, because
  // mus will roll out via experiment/Finch trial and showing the tray would
  // reveal the experiment state to users.
  const bool is_mash = Shell::GetAshConfig() == Config::MASH;
  FlagWarningTray* tray = Shell::GetPrimaryRootWindowController()
                              ->GetStatusAreaWidget()
                              ->flag_warning_tray_for_testing();
  EXPECT_EQ(tray != nullptr, is_mash);
}

}  // namespace
}  // namespace ash
