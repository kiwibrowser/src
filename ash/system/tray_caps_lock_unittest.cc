// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray_caps_lock.h"

#include "ash/system/message_center/notification_tray.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "ash/test/ash_test_base.h"

namespace ash {

class TrayCapsLockTest : public AshTestBase {
 public:
  TrayCapsLockTest() = default;
  ~TrayCapsLockTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    NotificationTray::DisableAnimationsForTest(true);
  }

  void TearDown() override {
    NotificationTray::DisableAnimationsForTest(false);
    AshTestBase::TearDown();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayCapsLockTest);
};

// Tests that the icon becomes visible when the tray controller toggles it.
TEST_F(TrayCapsLockTest, Visibility) {
  SystemTray* tray = GetPrimarySystemTray();
  TrayCapsLock* caps_lock = SystemTrayTestApi(tray).tray_caps_lock();

  // By default the icon isn't visible.
  EXPECT_FALSE(caps_lock->tray_view()->visible());

  // Simulate turning on caps lock.
  caps_lock->OnCapsLockChanged(true);
  EXPECT_TRUE(caps_lock->tray_view()->visible());

  // Simulate turning off caps lock.
  caps_lock->OnCapsLockChanged(false);
  EXPECT_FALSE(caps_lock->tray_view()->visible());
}

}  // namespace ash
