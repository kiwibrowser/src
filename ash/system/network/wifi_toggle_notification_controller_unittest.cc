// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/wifi_toggle_notification_controller.h"

#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/test/ash_test_base.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_handler.h"
#include "components/prefs/testing_pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"

using message_center::MessageCenter;

namespace ash {

class WifiToggleNotificationControllerTest : public AshTestBase {
 public:
  WifiToggleNotificationControllerTest() = default;
  ~WifiToggleNotificationControllerTest() override = default;

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
    base::RunLoop().RunUntilIdle();
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

  DISALLOW_COPY_AND_ASSIGN(WifiToggleNotificationControllerTest);
};

// Verifies that toggling Wi-Fi (usually via keyboard) shows a notification.
TEST_F(WifiToggleNotificationControllerTest, ToggleWifi) {
  // No notifications at startup.
  ASSERT_EQ(0u, MessageCenter::Get()->NotificationCount());

  // Simulate a user action to toggle Wi-Fi.
  Shell::Get()->system_tray_notifier()->NotifyRequestToggleWifi();

  // Notification was shown.
  EXPECT_EQ(1u, MessageCenter::Get()->NotificationCount());
  EXPECT_TRUE(MessageCenter::Get()->HasPopupNotifications());
  EXPECT_TRUE(MessageCenter::Get()->FindVisibleNotificationById("wifi-toggle"));
}

}  // namespace ash
