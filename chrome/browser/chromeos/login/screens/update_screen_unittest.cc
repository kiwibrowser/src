// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/update_screen.h"
#include "base/command_line.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "chrome/browser/chromeos/login/screens/mock_base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/mock_error_screen.h"
#include "chrome/browser/chromeos/login/screens/mock_update_screen.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_update_engine_client.h"
#include "chromeos/dbus/update_engine_client.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/portal_detector/mock_network_portal_detector.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "components/pairing/fake_host_pairing_controller.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AnyNumber;
using testing::Return;

namespace chromeos {

class UpdateScreenUnitTest : public testing::Test {
 public:
  UpdateScreenUnitTest()
      : fake_controller_(""),
        local_state_(TestingBrowserProcess::GetGlobal()) {}

  // Simulates an update being available (or not).
  // The parameter "update_screen" points to the currently active UpdateScreen.
  // The parameter "available" indicates whether an update is available.
  // The parameter "critical" indicates whether that update is critical.
  void SimulateUpdateAvailable(
      const std::unique_ptr<UpdateScreen>& update_screen,
      bool available,
      bool critical) {
    update_engine_status_.status =
        UpdateEngineClient::UPDATE_STATUS_CHECKING_FOR_UPDATE;
    fake_update_engine_client_->NotifyObserversThatStatusChanged(
        update_engine_status_);
    if (critical) {
      ASSERT_TRUE(available) << "Does not make sense for an update to be "
                                "critical if one is not even available.";
      update_screen->is_ignore_update_deadlines_ = true;
    }
    update_engine_status_.status =
        available ? UpdateEngineClient::UPDATE_STATUS_UPDATE_AVAILABLE
                  : UpdateEngineClient::UPDATE_STATUS_IDLE;
    fake_update_engine_client_->NotifyObserversThatStatusChanged(
        update_engine_status_);
  }

  // testing::Test:
  void SetUp() override {
    // Configure the browser to use Hands-Off Enrollment.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kEnterpriseEnableZeroTouchEnrollment, "hands-off");

    // Initialize objects needed by UpdateScreen.
    fake_update_engine_client_ = new FakeUpdateEngineClient();
    DBusThreadManager::GetSetterForTesting()->SetUpdateEngineClient(
        std::unique_ptr<UpdateEngineClient>(fake_update_engine_client_));
    NetworkHandler::Initialize();
    mock_network_portal_detector_ = new MockNetworkPortalDetector();
    network_portal_detector::SetNetworkPortalDetector(
        mock_network_portal_detector_);
    mock_error_screen_.reset(
        new MockErrorScreen(&mock_base_screen_delegate_, &mock_error_view_));

    // Ensure proper behavior of UpdateScreen's supporting objects.
    EXPECT_CALL(*mock_network_portal_detector_, IsEnabled())
        .Times(AnyNumber())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mock_base_screen_delegate_, GetErrorScreen())
        .Times(AnyNumber())
        .WillRepeatedly(Return(mock_error_screen_.get()));
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetShuttingDown(true);
    update_screen_.reset();
    mock_error_screen_.reset();
    network_portal_detector::Shutdown();
    NetworkHandler::Shutdown();
    DBusThreadManager::Shutdown();
  }

 protected:
  // A pointer to the UpdateScreen used in this test.
  std::unique_ptr<UpdateScreen> update_screen_;

  // Accessory objects needed by UpdateScreen.
  MockBaseScreenDelegate mock_base_screen_delegate_;
  MockUpdateView mock_view_;
  MockNetworkErrorView mock_error_view_;
  UpdateEngineClient::Status update_engine_status_;
  pairing_chromeos::FakeHostPairingController fake_controller_;
  std::unique_ptr<MockErrorScreen> mock_error_screen_;
  MockNetworkPortalDetector* mock_network_portal_detector_;
  FakeUpdateEngineClient* fake_update_engine_client_;

 private:
  // Test versions of core browser infrastructure.
  content::TestBrowserThreadBundle threads_;
  ScopedTestingLocalState local_state_;

  DISALLOW_COPY_AND_ASSIGN(UpdateScreenUnitTest);
};

TEST_F(UpdateScreenUnitTest, HandlesNoUpdate) {
  // Set expectation that UpdateScreen will exit successfully
  // with code UPDATE_NOUPDATE.
  EXPECT_CALL(mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);

  // DUT reaches UpdateScreen.
  update_screen_.reset(new UpdateScreen(&mock_base_screen_delegate_,
                                        &mock_view_, &fake_controller_));
  update_screen_->StartNetworkCheck();

  // Verify that the DUT checks for an update.
  EXPECT_EQ(fake_update_engine_client_->request_update_check_call_count(), 1);

  // No updates are available.
  SimulateUpdateAvailable(update_screen_, false /* available */,
                          false /* critical */);
}

TEST_F(UpdateScreenUnitTest, HandlesNonCriticalUpdate) {
  // Set expectation that UpdateScreen will exit successfully
  // with code UPDATE_NOUPDATE. No, this is not a typo.
  // UPDATE_NOUPDATE means that either there was no update
  // or there was a non-critical update.
  EXPECT_CALL(mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);

  // DUT reaches UpdateScreen.
  update_screen_.reset(new UpdateScreen(&mock_base_screen_delegate_,
                                        &mock_view_, &fake_controller_));
  update_screen_->StartNetworkCheck();

  // Verify that the DUT checks for an update.
  EXPECT_EQ(fake_update_engine_client_->request_update_check_call_count(), 1);

  // A non-critical update is available.
  SimulateUpdateAvailable(update_screen_, true /* available */,
                          false /* critical */);
}

TEST_F(UpdateScreenUnitTest, HandlesCriticalUpdate) {
  // Set expectation that UpdateScreen does not exit.
  // This is the case because a critical update mandates reboot.
  EXPECT_CALL(mock_base_screen_delegate_, OnExit(_, _, _)).Times(0);

  // DUT reaches UpdateScreen.
  update_screen_.reset(new UpdateScreen(&mock_base_screen_delegate_,
                                        &mock_view_, &fake_controller_));
  update_screen_->StartNetworkCheck();

  // Verify that the DUT checks for an update.
  EXPECT_EQ(fake_update_engine_client_->request_update_check_call_count(), 1);

  // An update is available, and it's critical!
  SimulateUpdateAvailable(update_screen_, true /* available */,
                          true /* critical */);
}

}  // namespace chromeos
