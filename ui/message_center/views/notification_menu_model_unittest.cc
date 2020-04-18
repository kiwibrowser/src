// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/notification_menu_model.h"

#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/menu_model.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/ui_controller.h"

namespace message_center {
namespace {

class TestNotificationDelegate : public NotificationDelegate {
 public:
  TestNotificationDelegate() = default;

 private:
  ~TestNotificationDelegate() override = default;

  DISALLOW_COPY_AND_ASSIGN(TestNotificationDelegate);
};

}  // namespace

class NotificationMenuModelTest : public testing::Test {
 public:
  NotificationMenuModelTest() = default;
  ~NotificationMenuModelTest() override = default;

  void SetUp() override {
    MessageCenter::Initialize();
    message_center_ = MessageCenter::Get();
  }

  void TearDown() override {
    message_center_ = NULL;
    MessageCenter::Shutdown();
  }

 protected:
  NotifierId DummyNotifierId() { return NotifierId(); }

  Notification* AddNotification(const std::string& id) {
    return AddNotification(id, DummyNotifierId());
  }

  Notification* AddNotification(const std::string& id, NotifierId notifier_id) {
    std::unique_ptr<Notification> notification(new Notification(
        NOTIFICATION_TYPE_SIMPLE, id,
        base::ASCIIToUTF16("Test Web Notification"),
        base::ASCIIToUTF16("Notification message body."), gfx::Image(),
        base::ASCIIToUTF16("www.test.org"), GURL(), notifier_id,
        RichNotificationData(), new TestNotificationDelegate()));
    Notification* notification_ptr = notification.get();
    message_center_->AddNotification(std::move(notification));
    return notification_ptr;
  }
  MessageCenter* message_center_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationMenuModelTest);
};

TEST_F(NotificationMenuModelTest, ContextMenuTestWithMessageCenter) {
  const std::string id1 = "id1";
  const std::string id2 = "id2";
  const std::string id3 = "id3";
  AddNotification(id1);

  base::string16 display_source = base::ASCIIToUTF16("www.test.org");
  NotifierId notifier_id = DummyNotifierId();

  NotifierId notifier_id2(NotifierId::APPLICATION, "sample-app");
  std::unique_ptr<Notification> notification = std::make_unique<Notification>(
      NOTIFICATION_TYPE_SIMPLE, id2,
      base::ASCIIToUTF16("Test Web Notification"),
      base::ASCIIToUTF16("Notification message body."), gfx::Image(),
      display_source, GURL(), notifier_id2, RichNotificationData(),
      new TestNotificationDelegate());

  std::unique_ptr<ui::MenuModel> model(
      std::make_unique<NotificationMenuModel>(*notification));
  message_center_->AddNotification(std::move(notification));

  AddNotification(id3);
  EXPECT_EQ(1, model->GetItemCount());

  // The first item is to disable notifications from the notifier id. It also
  // removes the notification.
  EXPECT_EQ(3u, message_center_->GetVisibleNotifications().size());
  model->ActivatedAt(0);
  EXPECT_EQ(2u, message_center_->GetVisibleNotifications().size());
}

}  // namespace message_center
