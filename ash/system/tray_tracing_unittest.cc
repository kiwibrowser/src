// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray_tracing.h"

#include "ash/shell.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"

namespace ash {
namespace {

using TrayTracingTest = AshTestBase;

// Tests that the icon becomes visible when the tray controller toggles it.
TEST_F(TrayTracingTest, Visibility) {
  SystemTray* tray = GetPrimarySystemTray();
  TrayTracing* tray_tracing = SystemTrayTestApi(tray).tray_tracing();

  // The system starts with tracing off, so the icon isn't visible.
  EXPECT_FALSE(tray_tracing->tray_view()->visible());

  // Simulate turning on tracing.
  SystemTrayController* controller = Shell::Get()->system_tray_controller();
  controller->SetPerformanceTracingIconVisible(true);
  EXPECT_TRUE(tray_tracing->tray_view()->visible());

  // Simulate turning off tracing.
  controller->SetPerformanceTracingIconVisible(false);
  EXPECT_FALSE(tray_tracing->tray_view()->visible());
}

}  // namespace
}  // namespace ash
