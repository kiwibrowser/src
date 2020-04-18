// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/chrome_ash_message_center_client.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/message_center/public/cpp/notification_delegate.h"

namespace {

class ChromeAshMessageCenterClientBrowserTest : public InProcessBrowserTest {
 public:
  ChromeAshMessageCenterClientBrowserTest() = default;
  ~ChromeAshMessageCenterClientBrowserTest() override = default;

  // InProcessBrowserTest overrides.
  void SetUpInProcessBrowserTestFixture() override {
    scoped_feature_list_.InitWithFeatures({features::kNativeNotifications}, {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAshMessageCenterClientBrowserTest);
};

class TestNotificationDelegate : public message_center::NotificationDelegate {
 public:
  TestNotificationDelegate() {}

  void Wait() { run_loop_.Run(); }

  void Close(bool by_user) override {
    close_count_++;
    run_loop_.Quit();
  }

  int close_count() const { return close_count_; }

 private:
  ~TestNotificationDelegate() override {}

  base::RunLoop run_loop_;
  int close_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestNotificationDelegate);
};

// Regression test for https://crbug.com/825141 that verifies out-of-order
// Display/Close pairs are handled correctly.
IN_PROC_BROWSER_TEST_F(ChromeAshMessageCenterClientBrowserTest,
                       DisplayCloseOrdering) {
  auto delegate = base::MakeRefCounted<TestNotificationDelegate>();
  const std::string id("notification_identifier");
  message_center::Notification notification(
      message_center::NOTIFICATION_TYPE_SIMPLE, id, base::string16(),
      base::string16(), gfx::Image(), base::ASCIIToUTF16("display_source"),
      GURL(),
      message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                 "notifier_id"),
      {}, delegate);

  auto* display_service =
      NotificationDisplayService::GetForProfile(browser()->profile());
  display_service->Display(NotificationHandler::Type::TRANSIENT, notification);
  display_service->Close(NotificationHandler::Type::TRANSIENT,
                         notification.id());
  // The Close callback should be fired asynchronously, so there is no close
  // yet.
  EXPECT_EQ(0, delegate->close_count());

  display_service->Display(NotificationHandler::Type::TRANSIENT, notification);
  display_service->Close(NotificationHandler::Type::TRANSIENT,
                         notification.id());
  ChromeAshMessageCenterClient::FlushForTesting();

  // Only one close logged because Display was called again before the first
  // close arrived.
  EXPECT_EQ(1, delegate->close_count());
}

}  // namespace
