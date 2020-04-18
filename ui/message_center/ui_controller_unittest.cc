// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/ui_controller.h"

#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/models/menu_model.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"

using base::ASCIIToUTF16;

namespace message_center {
namespace {

class TestNotificationDelegate : public NotificationDelegate {
 public:
  TestNotificationDelegate() = default;

 private:
  ~TestNotificationDelegate() override = default;

  DISALLOW_COPY_AND_ASSIGN(TestNotificationDelegate);
};

class MockDelegate : public UiDelegate {
 public:
  MockDelegate() {}
  ~MockDelegate() override {}
  void OnMessageCenterContentsChanged() override {}
  bool ShowPopups() override { return show_message_center_success_; }
  void HidePopups() override {}
  bool ShowMessageCenter(bool show_by_click) override {
    return show_popups_success_;
  }
  void HideMessageCenter() override {}
  bool ShowNotifierSettings() override { return true; }

  bool show_popups_success_ = true;
  bool show_message_center_success_ = true;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDelegate);
};

}  // namespace

class UiControllerTest : public testing::Test {
 public:
  UiControllerTest() {}
  ~UiControllerTest() override {}

  void SetUp() override {
    MessageCenter::Initialize();
    delegate_.reset(new MockDelegate);
    message_center_ = MessageCenter::Get();
    ui_controller_.reset(new UiController(delegate_.get()));
  }

  void TearDown() override {
    ui_controller_.reset();
    delegate_.reset();
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
        NOTIFICATION_TYPE_SIMPLE, id, ASCIIToUTF16("Test Web Notification"),
        ASCIIToUTF16("Notification message body."), gfx::Image(),
        ASCIIToUTF16("www.test.org"), GURL(), notifier_id,
        RichNotificationData(), new TestNotificationDelegate()));
    Notification* notification_ptr = notification.get();
    message_center_->AddNotification(std::move(notification));
    return notification_ptr;
  }
  std::unique_ptr<MockDelegate> delegate_;
  std::unique_ptr<UiController> ui_controller_;
  MessageCenter* message_center_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UiControllerTest);
};

TEST_F(UiControllerTest, BasicMessageCenter) {
  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  bool shown =
      ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);
  EXPECT_TRUE(shown);

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_TRUE(ui_controller_->message_center_visible());

  ui_controller_->HideMessageCenterBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_TRUE(ui_controller_->message_center_visible());

  ui_controller_->HideMessageCenterBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());
}

TEST_F(UiControllerTest, BasicPopup) {
  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->ShowPopupBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  AddNotification("BasicPopup");

  ASSERT_TRUE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->HidePopupBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());
}

TEST_F(UiControllerTest, MessageCenterClosesPopups) {
  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  AddNotification("MessageCenterClosesPopups");

  ASSERT_TRUE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  bool shown =
      ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);
  EXPECT_TRUE(shown);

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_TRUE(ui_controller_->message_center_visible());

  // The notification is queued if it's added when message center is visible.
  AddNotification("MessageCenterClosesPopups2");

  ui_controller_->ShowPopupBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_TRUE(ui_controller_->message_center_visible());

  ui_controller_->HideMessageCenterBubble();

  // There is no queued notification.
  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);
  ui_controller_->HideMessageCenterBubble();
  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());
}

TEST_F(UiControllerTest, MessageCenterReopenPopupsForSystemPriority) {
  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  std::unique_ptr<Notification> notification(new Notification(
      NOTIFICATION_TYPE_SIMPLE, "MessageCenterReopnPopupsForSystemPriority",
      ASCIIToUTF16("Test Web Notification"),
      ASCIIToUTF16("Notification message body."), gfx::Image(),
      ASCIIToUTF16("www.test.org"), GURL(), DummyNotifierId(),
      RichNotificationData(), NULL /* delegate */));
  notification->SetSystemPriority();
  message_center_->AddNotification(std::move(notification));

  ASSERT_TRUE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  bool shown =
      ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);
  EXPECT_TRUE(shown);

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_TRUE(ui_controller_->message_center_visible());

  ui_controller_->HideMessageCenterBubble();

  ASSERT_TRUE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());
}

TEST_F(UiControllerTest, ShowBubbleFails) {
  // Now the delegate will signal that it was unable to show a bubble.
  delegate_->show_popups_success_ = false;
  delegate_->show_message_center_success_ = false;

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  AddNotification("ShowBubbleFails");

  ui_controller_->ShowPopupBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  bool shown =
      ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);
  EXPECT_FALSE(shown);

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->HideMessageCenterBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->ShowMessageCenterBubble(false /* show_by_click */);

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());

  ui_controller_->HidePopupBubble();

  ASSERT_FALSE(ui_controller_->popups_visible());
  ASSERT_FALSE(ui_controller_->message_center_visible());
}

}  // namespace message_center
