// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/update/update_notification_controller.h"

#include "ash/public/cpp/ash_features.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/test/ash_test_base.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "ui/message_center/message_center.h"

namespace ash {

class UpdateNotificationControllerTest : public AshTestBase {
 public:
  UpdateNotificationControllerTest() = default;
  ~UpdateNotificationControllerTest() override = default;

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kSystemTrayUnified);
    AshTestBase::SetUp();
  }

 protected:
  bool HasNotification() {
    return message_center::MessageCenter::Get()->FindVisibleNotificationById(
        UpdateNotificationController::kNotificationId);
  }

  std::string GetNotificationTitle() {
    return base::UTF16ToUTF8(
        message_center::MessageCenter::Get()
            ->FindVisibleNotificationById(
                UpdateNotificationController::kNotificationId)
            ->title());
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(UpdateNotificationControllerTest);
};

// Tests that the update icon becomes visible when an update becomes
// available.
TEST_F(UpdateNotificationControllerTest, VisibilityAfterUpdate) {
  // The system starts with no update pending, so the notification isn't
  // visible.
  EXPECT_FALSE(HasNotification());

  // Simulate an update.
  Shell::Get()->system_tray_controller()->ShowUpdateIcon(
      mojom::UpdateSeverity::LOW, false, mojom::UpdateType::SYSTEM);

  // The notification is now visible.
  ASSERT_TRUE(HasNotification());
  EXPECT_EQ("Restart to update", GetNotificationTitle());
}

TEST_F(UpdateNotificationControllerTest, VisibilityAfterFlashUpdate) {
  // The system starts with no update pending, so the notification isn't
  // visible.
  EXPECT_FALSE(HasNotification());

  // Simulate an update.
  Shell::Get()->system_tray_controller()->ShowUpdateIcon(
      mojom::UpdateSeverity::LOW, false, mojom::UpdateType::FLASH);

  // The notification is now visible.
  ASSERT_TRUE(HasNotification());
  EXPECT_EQ("Restart to update Adobe Flash Player", GetNotificationTitle());
}

// Tests that the update icon's visibility after an update becomes
// available for downloading over cellular connection.
TEST_F(UpdateNotificationControllerTest,
       VisibilityAfterUpdateOverCellularAvailable) {
  // The system starts with no update pending, so the notification isn't
  // visible.
  EXPECT_FALSE(HasNotification());

  // Simulate an update available for downloading over cellular connection.
  Shell::Get()
      ->system_tray_controller()
      ->SetUpdateOverCellularAvailableIconVisible(true);

  // The notification is now visible.
  ASSERT_TRUE(HasNotification());
  EXPECT_EQ("Click to view update details", GetNotificationTitle());

  // Simulate the user's one time permission on downloading the update is
  // granted.
  Shell::Get()
      ->system_tray_controller()
      ->SetUpdateOverCellularAvailableIconVisible(false);

  // The notification disappears.
  EXPECT_FALSE(HasNotification());
}

}  // namespace ash
