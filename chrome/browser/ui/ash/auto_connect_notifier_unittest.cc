// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/auto_connect_notifier.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/timer/mock_timer.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/auto_connect_handler.h"
#include "chromeos/network/network_connection_handler.h"
#include "chromeos/network/network_state_test.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "third_party/cros_system_api/dbus/shill/dbus-constants.h"
#include "ui/message_center/public/cpp/notification.h"

namespace {

const char kAutoConnectNotificationId[] =
    "cros_auto_connect_notifier_ids.connected_to_network";

const char kTestServicePath[] = "testServicePath";

const char kWifiNetworkGuid[] = "wifiNetworkGuid";

std::string CreateWifiConfigurationJsonString() {
  std::stringstream ss;
  ss << "{"
     << "  \"GUID\": \"" << kWifiNetworkGuid << "\","
     << "  \"Type\": \"" << shill::kTypeWifi << "\","
     << "  \"State\": \"" << shill::kStateOnline << "\""
     << "}";
  return ss.str();
}

class TestNetworkConnectionHandler : public chromeos::NetworkConnectionHandler {
 public:
  TestNetworkConnectionHandler() : chromeos::NetworkConnectionHandler() {}
  ~TestNetworkConnectionHandler() override = default;

  void NotifyConnectToNetworkRequested() {
    for (auto& observer : observers_)
      observer.ConnectToNetworkRequested(kTestServicePath);
  }

  // chromeos::NetworkConnectionHandler:
  void DisconnectNetwork(
      const std::string& service_path,
      const base::Closure& success_callback,
      const chromeos::network_handler::ErrorCallback& error_callback) override {
  }

  void ConnectToNetwork(
      const std::string& service_path,
      const base::Closure& success_callback,
      const chromeos::network_handler::ErrorCallback& error_callback,
      bool check_error_state,
      chromeos::ConnectCallbackMode mode) override {}

  bool HasConnectingNetwork(const std::string& service_path) override {
    return false;
  }

  bool HasPendingConnectRequest() override { return false; }

  void Init(
      chromeos::NetworkStateHandler* network_state_handler,
      chromeos::NetworkConfigurationHandler* network_configuration_handler,
      chromeos::ManagedNetworkConfigurationHandler*
          managed_network_configuration_handler) override {}
};

}  // namespace

class AutoConnectNotifierTest : public chromeos::NetworkStateTest {
 protected:
  // Nested within AutoConnectNotifierTest for visibility into
  // AutoConnectHandler's constructor.
  class TestAutoConnectHandler : public chromeos::AutoConnectHandler {
   public:
    TestAutoConnectHandler() = default;
    ~TestAutoConnectHandler() override = default;

    // Make chromeos::AutoConnectHandler::NotifyAutoConnectInitiated() public.
    using chromeos::AutoConnectHandler::NotifyAutoConnectInitiated;
  };

  AutoConnectNotifierTest() = default;

  void SetUp() override {
    chromeos::DBusThreadManager::Initialize();
    chromeos::NetworkStateTest::SetUp();

    TestingProfile::Builder builder;
    profile_ = builder.Build();

    display_service_ =
        std::make_unique<NotificationDisplayServiceTester>(profile_.get());
    test_network_connection_handler_ =
        base::WrapUnique(new TestNetworkConnectionHandler());
    test_auto_connect_handler_ = base::WrapUnique(new TestAutoConnectHandler());

    auto_connect_notifier_ = std::make_unique<AutoConnectNotifier>(
        profile_.get(), test_network_connection_handler_.get(),
        network_state_handler(), test_auto_connect_handler_.get());

    mock_timer_ = new base::MockTimer(true /* retain_user_task */,
                                      false /* is_repeating */);
    auto_connect_notifier_->SetTimerForTesting(base::WrapUnique(mock_timer_));
  }

  void TearDown() override {
    auto_connect_notifier_.reset();

    ShutdownNetworkState();
    chromeos::NetworkStateTest::TearDown();
    chromeos::DBusThreadManager::Shutdown();
  }

  void SuccessfullyJoinWifiNetwork() {
    ConfigureService(CreateWifiConfigurationJsonString());
    base::RunLoop().RunUntilIdle();
  }

  const content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<NotificationDisplayServiceTester> display_service_;
  std::unique_ptr<TestNetworkConnectionHandler>
      test_network_connection_handler_;
  std::unique_ptr<TestAutoConnectHandler> test_auto_connect_handler_;
  base::MockTimer* mock_timer_;

  std::unique_ptr<AutoConnectNotifier> auto_connect_notifier_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AutoConnectNotifierTest);
};

TEST_F(AutoConnectNotifierTest, NoExplicitConnectionRequested) {
  test_auto_connect_handler_->NotifyAutoConnectInitiated(
      chromeos::AutoConnectHandler::AUTO_CONNECT_REASON_POLICY_APPLIED);
  SuccessfullyJoinWifiNetwork();

  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kAutoConnectNotificationId);
  EXPECT_FALSE(notification);
}

TEST_F(AutoConnectNotifierTest, AutoConnectDueToLoginOnly) {
  test_network_connection_handler_->NotifyConnectToNetworkRequested();
  test_auto_connect_handler_->NotifyAutoConnectInitiated(
      chromeos::AutoConnectHandler::AUTO_CONNECT_REASON_LOGGED_IN);
  SuccessfullyJoinWifiNetwork();

  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kAutoConnectNotificationId);
  EXPECT_FALSE(notification);
}

TEST_F(AutoConnectNotifierTest, NoConnectionBeforeTimerExpires) {
  test_network_connection_handler_->NotifyConnectToNetworkRequested();
  test_auto_connect_handler_->NotifyAutoConnectInitiated(
      chromeos::AutoConnectHandler::AUTO_CONNECT_REASON_POLICY_APPLIED);

  // No connection occurs.
  mock_timer_->Fire();

  // Connect after the timer fires; since the connection did not occur before
  // the timeout, no notification should be displayed.
  SuccessfullyJoinWifiNetwork();

  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kAutoConnectNotificationId);
  EXPECT_FALSE(notification);
}

TEST_F(AutoConnectNotifierTest, NotificationDisplayed) {
  test_network_connection_handler_->NotifyConnectToNetworkRequested();
  test_auto_connect_handler_->NotifyAutoConnectInitiated(
      chromeos::AutoConnectHandler::AUTO_CONNECT_REASON_POLICY_APPLIED);
  SuccessfullyJoinWifiNetwork();

  base::Optional<message_center::Notification> notification =
      display_service_->GetNotification(kAutoConnectNotificationId);
  ASSERT_TRUE(notification);
  EXPECT_EQ(kAutoConnectNotificationId, notification->id());
}
