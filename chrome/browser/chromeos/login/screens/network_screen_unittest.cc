// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "chrome/browser/chromeos/input_method/input_method_configuration.h"
#include "chrome/browser/chromeos/input_method/mock_input_method_manager_impl.h"
#include "chrome/browser/chromeos/login/mock_network_state_helper.h"
#include "chrome/browser/chromeos/login/screens/mock_base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/mock_model_view_channel.h"
#include "chrome/browser/chromeos/login/screens/mock_network_screen.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/system/fake_statistics_provider.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ime/chromeos/mock_component_extension_ime_manager.h"

using testing::_;
using testing::AnyNumber;
using testing::Return;

namespace chromeos {

class NetworkScreenUnitTest : public testing::Test {
 public:
  NetworkScreenUnitTest() {}

  // testing::Test:
  void SetUp() override {
    // Initialize the thread manager.
    DBusThreadManager::Initialize();

    // Configure the browser to use Hands-Off Enrollment.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kEnterpriseEnableZeroTouchEnrollment, "hands-off");

    // Replace the regular InputMethodManager with a mock.
    input_method::MockInputMethodManagerImpl* mock_input_manager =
        new input_method::MockInputMethodManagerImpl();
    mock_input_manager->SetComponentExtensionIMEManager(
        std::unique_ptr<MockComponentExtensionIMEManager>(
            new MockComponentExtensionIMEManager()));
    input_method::InitializeForTesting(mock_input_manager);

    // Create the NetworkScreen we will use for testing.
    network_screen_.reset(
        new NetworkScreen(&mock_base_screen_delegate_, nullptr, &mock_view_));
    network_screen_->set_model_view_channel(&mock_channel_);
    mock_network_state_helper_ = new login::MockNetworkStateHelper();
    network_screen_->SetNetworkStateHelperForTest(mock_network_state_helper_);
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetShuttingDown(true);
    network_screen_.reset();
    input_method::Shutdown();
    DBusThreadManager::Shutdown();
  }

 protected:
  // A pointer to the NetworkScreen.
  std::unique_ptr<NetworkScreen> network_screen_;

  // Accessory objects needed by NetworkScreen.
  MockBaseScreenDelegate mock_base_screen_delegate_;
  login::MockNetworkStateHelper* mock_network_state_helper_ = nullptr;

 private:
  // Test versions of core browser infrastructure.
  content::TestBrowserThreadBundle threads_;

  // More accessory objects needed by NetworkScreen.
  MockNetworkView mock_view_;
  MockModelViewChannel mock_channel_;

  // Scoped test versions of required global objects.
  ScopedTestDeviceSettingsService device_settings_;
  ScopedTestCrosSettings cros_settings_;
  system::ScopedFakeStatisticsProvider provider_;

  DISALLOW_COPY_AND_ASSIGN(NetworkScreenUnitTest);
};

TEST_F(NetworkScreenUnitTest, ContinuesAutomatically) {
  // Set expectation that NetworkScreen will finish.
  EXPECT_CALL(mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::NETWORK_CONNECTED, _))
      .Times(1);

  // Simulate a network connection.
  EXPECT_CALL(*mock_network_state_helper_, IsConnected())
      .Times(AnyNumber())
      .WillRepeatedly((Return(true)));
  network_screen_->UpdateStatus();

  // Check that we continued once
  EXPECT_EQ(1, network_screen_->continue_attempts_);
}

TEST_F(NetworkScreenUnitTest, ContinuesOnlyOnce) {
  // Set expectation that NetworkScreen will finish.
  EXPECT_CALL(mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::NETWORK_CONNECTED, _))
      .Times(1);

  // Connect to network "net0".
  EXPECT_CALL(*mock_network_state_helper_, GetCurrentNetworkName())
      .Times(AnyNumber())
      .WillRepeatedly(Return(base::ASCIIToUTF16("net0")));
  EXPECT_CALL(*mock_network_state_helper_, IsConnected())
      .Times(AnyNumber())
      .WillRepeatedly(Return(true));

  // Stop waiting for net0.
  network_screen_->StopWaitingForConnection(base::ASCIIToUTF16("net0"));

  // Check that we have continued exactly once.
  ASSERT_EQ(1, network_screen_->continue_attempts_);

  // Stop waiting for another network, net1.
  network_screen_->StopWaitingForConnection(base::ASCIIToUTF16("net1"));

  // Check that we have still continued only once.
  EXPECT_EQ(1, network_screen_->continue_attempts_);
}

}  // namespace chromeos
