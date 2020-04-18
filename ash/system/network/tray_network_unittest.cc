// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/tray_network.h"

#include "ash/login_status.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/network/network_list.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_handler.h"
#include "components/prefs/testing_pref_service.h"

namespace ash {
namespace {

class TrayNetworkTest : public AshTestBase {
 public:
  TrayNetworkTest() = default;
  ~TrayNetworkTest() override = default;

  // testing::Test:
  void SetUp() override {
    chromeos::DBusThreadManager::Initialize();
    // Initializing NetworkHandler before ash is more like production.
    chromeos::NetworkHandler::Initialize();
    AshTestBase::SetUp();
    // Mash doesn't do this yet, so don't do it in tests either.
    // http://crbug.com/718072
    if (Shell::GetAshConfig() != Config::MASH) {
      chromeos::NetworkHandler::Get()->InitializePrefServices(&profile_prefs_,
                                                              &local_state_);
    }
    // Networking stubs may have asynchronous initialization.
    RunAllPendingInMessageLoop();
  }

  void TearDown() override {
    // This roughly matches production shutdown order.
    if (Shell::GetAshConfig() != Config::MASH) {
      chromeos::NetworkHandler::Get()->ShutdownPrefServices();
    }
    AshTestBase::TearDown();
    chromeos::NetworkHandler::Shutdown();
    chromeos::DBusThreadManager::Shutdown();
  }

 private:
  TestingPrefServiceSimple profile_prefs_;
  TestingPrefServiceSimple local_state_;

  DISALLOW_COPY_AND_ASSIGN(TrayNetworkTest);
};

// Verifies that the network views can be created.
TEST_F(TrayNetworkTest, Basics) {
  // Open the system tray menu.
  SystemTray* system_tray = GetPrimarySystemTray();
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();

  // Show network details.
  TrayNetwork* tray_network = SystemTrayTestApi(system_tray).tray_network();
  const int close_delay_in_seconds = 0;
  system_tray->ShowDetailedView(tray_network, close_delay_in_seconds,
                                BUBBLE_USE_EXISTING);
  RunAllPendingInMessageLoop();

  // Network details view was created.
  ASSERT_TRUE(tray_network->detailed());
  EXPECT_TRUE(tray_network->detailed()->visible());
}

// Open network info bubble and close network detailed view. Confirm that it
// doesn't crash.
TEST_F(TrayNetworkTest, NetworkInfoBubble) {
  // Open the system tray menu.
  SystemTray* system_tray = GetPrimarySystemTray();
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, true /* show_by_click */);
  RunAllPendingInMessageLoop();

  // Show network details.
  TrayNetwork* tray_network = SystemTrayTestApi(system_tray).tray_network();
  const int close_delay_in_seconds = 0;
  system_tray->ShowDetailedView(tray_network, close_delay_in_seconds,
                                BUBBLE_USE_EXISTING);
  RunAllPendingInMessageLoop();

  // Show info bubble.
  tray_network->detailed()->ToggleInfoBubbleForTesting();

  // TearDown() should close the bubble and not crash.
}

}  // namespace
}  // namespace ash
