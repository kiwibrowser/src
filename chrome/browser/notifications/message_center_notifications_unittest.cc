// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/test/test_timeouts.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/message_center_notification_manager.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/message_center/fake_ui_delegate.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/message_center_types.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "ui/message_center/ui_controller.h"

namespace message_center {

class MessageCenterNotificationManagerTest : public BrowserWithTestWindowTest {
 public:
  MessageCenterNotificationManagerTest() {}

 protected:
  void SetUp() override {
#if !defined(OS_CHROMEOS)
    // Ash shell initializes the message center.
    MessageCenter::Initialize();
#endif

    BrowserWithTestWindowTest::SetUp();
    message_center_ = MessageCenter::Get();
    delegate_ = new FakeUiDelegate();
    notification_manager()->SetUiDelegateForTest(delegate_);
  }

  void TearDown() override {
    BrowserWithTestWindowTest::TearDown();
#if !defined(OS_CHROMEOS)
    // Shutdown the message center if we initialized it manually.
    MessageCenter::Shutdown();
#endif
  }

  MessageCenterNotificationManager* notification_manager() {
    return (MessageCenterNotificationManager*)
        g_browser_process->notification_ui_manager();
  }

  MessageCenter* message_center() { return message_center_; }

  const Notification GetANotification(const std::string& id) {
    return Notification(
        message_center::NOTIFICATION_TYPE_SIMPLE, id, base::string16(),
        base::string16(), gfx::Image(), base::string16(),
        GURL("chrome-extension://adflkjsdflkdsfdsflkjdsflkdjfs"),
        NotifierId(NotifierId::APPLICATION, "adflkjsdflkdsfdsflkjdsflkdjfs"),
        message_center::RichNotificationData(),
        new message_center::NotificationDelegate());
  }

 private:
  MessageCenter* message_center_;
  FakeUiDelegate* delegate_;
};

TEST_F(MessageCenterNotificationManagerTest, SetupNotificationManager) {
  TestingProfile profile;
  notification_manager()->Add(GetANotification("test"), &profile);
}

TEST_F(MessageCenterNotificationManagerTest, AddNotificationOnShutdown) {
  TestingProfile profile;
  EXPECT_TRUE(message_center()->NotificationCount() == 0);
  notification_manager()->Add(GetANotification("test"), &profile);
  EXPECT_TRUE(message_center()->NotificationCount() == 1);

  // Verify the number of notifications does not increase when trying to add a
  // notifcation on shutdown.
  notification_manager()->StartShutdown();
  EXPECT_TRUE(message_center()->NotificationCount() == 0);
  notification_manager()->Add(GetANotification("test2"), &profile);
  EXPECT_TRUE(message_center()->NotificationCount() == 0);
}

TEST_F(MessageCenterNotificationManagerTest, UpdateNotification) {
  TestingProfile profile;
  EXPECT_TRUE(message_center()->NotificationCount() == 0);
  notification_manager()->Add(GetANotification("test"), &profile);
  EXPECT_TRUE(message_center()->NotificationCount() == 1);
  ASSERT_TRUE(
      notification_manager()->Update(GetANotification("test"), &profile));
  EXPECT_TRUE(message_center()->NotificationCount() == 1);
}

// Regression test for crbug.com/767868
TEST_F(MessageCenterNotificationManagerTest, GetAllIdsReturnsOriginalId) {
  TestingProfile profile;
  EXPECT_TRUE(message_center()->NotificationCount() == 0);
  notification_manager()->Add(GetANotification("test"), &profile);
  std::set<std::string> ids = notification_manager()->GetAllIdsByProfile(
      NotificationUIManager::GetProfileID(&profile));
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(*ids.begin(), "test");
}

}  // namespace message_center
