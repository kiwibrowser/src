// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/network/network_portal_notification_controller.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/network_state.h"

namespace chromeos {

namespace {

const char* const kNotificationId =
    NetworkPortalNotificationController::kNotificationId;

}  // namespace

// A BrowserWithTestWindowTest will set up profiles for us.
class NetworkPortalNotificationControllerTest
    : public BrowserWithTestWindowTest {
 public:
  NetworkPortalNotificationControllerTest() : controller_(nullptr) {}
  ~NetworkPortalNotificationControllerTest() override {}

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
    cl->AppendSwitch(switches::kEnableNetworkPortalNotification);

    display_service_ = std::make_unique<NotificationDisplayServiceTester>(
        ProfileHelper::GetSigninProfile());
  }

 protected:
  void OnPortalDetectionCompleted(
      const NetworkState* network,
      const NetworkPortalDetector::CaptivePortalState& state) {
    controller_.OnPortalDetectionCompleted(network, state);
  }

  bool HasNotification() {
    return !!display_service_->GetNotification(kNotificationId);
  }

  std::unique_ptr<NotificationDisplayServiceTester> display_service_;
  NetworkPortalNotificationController controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkPortalNotificationControllerTest);
};

TEST_F(NetworkPortalNotificationControllerTest, NetworkStateChanged) {
  NetworkState wifi("wifi");
  wifi.SetGuid("wifi");
  NetworkPortalDetector::CaptivePortalState wifi_state;

  // Notification is not displayed for online state.
  wifi_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE;
  wifi_state.response_code = 204;
  OnPortalDetectionCompleted(&wifi, wifi_state);
  ASSERT_FALSE(HasNotification());

  // Notification is displayed for portal state
  wifi_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL;
  wifi_state.response_code = 200;
  OnPortalDetectionCompleted(&wifi, wifi_state);
  ASSERT_TRUE(HasNotification());

  // Notification is closed for online state.
  wifi_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE;
  wifi_state.response_code = 204;
  OnPortalDetectionCompleted(&wifi, wifi_state);
  ASSERT_FALSE(HasNotification());
}

TEST_F(NetworkPortalNotificationControllerTest, NetworkChanged) {
  NetworkState wifi1("wifi1");
  wifi1.SetGuid("wifi1");
  NetworkPortalDetector::CaptivePortalState wifi1_state;
  wifi1_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL;
  wifi1_state.response_code = 200;
  OnPortalDetectionCompleted(&wifi1, wifi1_state);
  ASSERT_TRUE(HasNotification());

  display_service_->RemoveNotification(NotificationHandler::Type::TRANSIENT,
                                       kNotificationId, true /* by_user */);
  ASSERT_FALSE(HasNotification());

  // User already closed notification about portal state for this network,
  // so notification shouldn't be displayed second time.
  OnPortalDetectionCompleted(&wifi1, wifi1_state);
  ASSERT_FALSE(HasNotification());

  NetworkState wifi2("wifi2");
  wifi2.SetGuid("wifi2");
  NetworkPortalDetector::CaptivePortalState wifi2_state;
  wifi2_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE;
  wifi2_state.response_code = 204;

  // Second network is in online state, so there shouldn't be any
  // notifications.
  OnPortalDetectionCompleted(&wifi2, wifi2_state);
  ASSERT_FALSE(HasNotification());

  // User switches back to the first network, so notification should
  // be displayed.
  OnPortalDetectionCompleted(&wifi1, wifi1_state);
  ASSERT_TRUE(HasNotification());
}

TEST_F(NetworkPortalNotificationControllerTest, NotificationUpdated) {
  NetworkPortalDetector::CaptivePortalState portal_state;
  portal_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL;
  portal_state.response_code = 200;

  // First network is behind a captive portal, so notification should
  // be displayed.
  NetworkState wifi1("wifi1");
  wifi1.SetGuid("wifi1");
  wifi1.PropertyChanged("Name", base::Value("wifi1"));
  OnPortalDetectionCompleted(&wifi1, portal_state);
  ASSERT_TRUE(HasNotification());
  EXPECT_EQ(1u, display_service_
                    ->GetDisplayedNotificationsForType(
                        NotificationHandler::Type::TRANSIENT)
                    .size());
  const base::string16 initial_message =
      display_service_->GetNotification(kNotificationId)->message();

  // Second network is also behind a captive portal, so notification
  // should be updated.
  NetworkState wifi2("wifi2");
  wifi2.SetGuid("wifi2");
  wifi2.PropertyChanged("Name", base::Value("wifi2"));
  OnPortalDetectionCompleted(&wifi2, portal_state);
  ASSERT_TRUE(HasNotification());
  EXPECT_EQ(1u, display_service_
                    ->GetDisplayedNotificationsForType(
                        NotificationHandler::Type::TRANSIENT)
                    .size());
  EXPECT_NE(initial_message,
            display_service_->GetNotification(kNotificationId)->message());

  // User closes the notification.
  display_service_->RemoveNotification(NotificationHandler::Type::TRANSIENT,
                                       kNotificationId, true /* by_user */);
  ASSERT_FALSE(HasNotification());

  // Portal detector notified that second network is still behind captive
  // portal, but user already closed the notification, so there should
  // not be any notifications.
  OnPortalDetectionCompleted(&wifi2, portal_state);
  ASSERT_FALSE(HasNotification());

  // Network was switched (by shill or by user) to wifi1. Notification
  // should be displayed.
  OnPortalDetectionCompleted(&wifi1, portal_state);
  ASSERT_TRUE(HasNotification());
}

}  // namespace chromeos
