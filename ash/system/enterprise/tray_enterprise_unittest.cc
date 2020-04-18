// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/enterprise/tray_enterprise.h"

#include "ash/shell.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"

namespace ash {

using TrayEnterpriseTest = AshTestBase;

TEST_F(TrayEnterpriseTest, ItemVisible) {
  SystemTray* system_tray = GetPrimarySystemTray();
  TrayEnterprise* tray_enterprise =
      SystemTrayTestApi(system_tray).tray_enterprise();

  // By default there is no enterprise item in the menu.
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_FALSE(tray_enterprise->tray_view()->visible());
  system_tray->CloseBubble();

  // Simulate enterprise information becoming available.
  const bool active_directory = false;
  Shell::Get()->system_tray_controller()->SetEnterpriseDisplayDomain(
      "example.com", active_directory);

  // Enterprise managed devices show an item.
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray_enterprise->tray_view()->visible());
  system_tray->CloseBubble();
}

TEST_F(TrayEnterpriseTest, ItemVisibleForActiveDirectory) {
  SystemTray* system_tray = GetPrimarySystemTray();
  TrayEnterprise* tray_enterprise =
      SystemTrayTestApi(system_tray).tray_enterprise();

  // Simulate enterprise information becoming available. Active Directory
  // devices do not have a domain.
  const std::string empty_domain;
  const bool active_directory = true;
  Shell::Get()->system_tray_controller()->SetEnterpriseDisplayDomain(
      empty_domain, active_directory);

  // Active Directory managed devices show an item.
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray_enterprise->tray_view()->visible());
  system_tray->CloseBubble();
}

}  // namespace ash
