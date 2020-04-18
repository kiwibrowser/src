// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/update_screen.h"

#include <memory>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/screens/mock_base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/mock_error_screen.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/test/wizard_in_process_browser_test.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/net/network_portal_detector_test_impl.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_update_engine_client.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::Exactly;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;

namespace chromeos {

namespace {

const char kStubEthernetGuid[] = "eth0";
const char kStubWifiGuid[] = "wlan0";

}  // namespace

class UpdateScreenTest : public WizardInProcessBrowserTest {
 public:
  UpdateScreenTest()
      : WizardInProcessBrowserTest(OobeScreen::SCREEN_OOBE_UPDATE),
        fake_update_engine_client_(nullptr),
        network_portal_detector_(nullptr) {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    fake_update_engine_client_ = new FakeUpdateEngineClient;
    chromeos::DBusThreadManager::GetSetterForTesting()->SetUpdateEngineClient(
        std::unique_ptr<UpdateEngineClient>(fake_update_engine_client_));

    WizardInProcessBrowserTest::SetUpInProcessBrowserTestFixture();

    // Setup network portal detector to return online state for both
    // ethernet and wifi networks. Ethernet is an active network by
    // default.
    network_portal_detector_ = new NetworkPortalDetectorTestImpl();
    network_portal_detector::InitializeForTesting(network_portal_detector_);
    NetworkPortalDetector::CaptivePortalState online_state;
    online_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE;
    online_state.response_code = 204;
    SetDefaultNetwork(kStubEthernetGuid);
    SetDetectionResults(kStubEthernetGuid, online_state);
    SetDetectionResults(kStubWifiGuid, online_state);
  }

  void SetUpOnMainThread() override {
    mock_base_screen_delegate_.reset(new MockBaseScreenDelegate());
    mock_network_error_view_.reset(new MockNetworkErrorView());
    mock_error_screen_.reset(new MockErrorScreen(
        mock_base_screen_delegate_.get(), mock_network_error_view_.get()));
    EXPECT_CALL(*mock_base_screen_delegate_, ShowCurrentScreen())
        .Times(AnyNumber());
    EXPECT_CALL(*mock_base_screen_delegate_, GetErrorScreen())
        .Times(AnyNumber())
        .WillRepeatedly(Return(mock_error_screen_.get()));

    WizardInProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(WizardController::default_controller() != nullptr);
    update_screen_ = UpdateScreen::Get(
        WizardController::default_controller()->screen_manager());
    ASSERT_TRUE(update_screen_ != nullptr);
    ASSERT_EQ(WizardController::default_controller()->current_screen(),
              update_screen_);
    update_screen_->base_screen_delegate_ = mock_base_screen_delegate_.get();
  }

  void TearDownOnMainThread() override {
    WizardInProcessBrowserTest::TearDownOnMainThread();
    mock_error_screen_.reset();
    mock_network_error_view_.reset();
  }

  void TearDownInProcessBrowserTestFixture() override {
    network_portal_detector::Shutdown();
    WizardInProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void SetDefaultNetwork(const std::string& guid) {
    DCHECK(network_portal_detector_);
    network_portal_detector_->SetDefaultNetworkForTesting(guid);
  }

  void SetDetectionResults(
      const std::string& guid,
      const NetworkPortalDetector::CaptivePortalState& state) {
    DCHECK(network_portal_detector_);
    network_portal_detector_->SetDetectionResultsForTesting(guid, state);
  }

  void NotifyPortalDetectionCompleted() {
    DCHECK(network_portal_detector_);
    network_portal_detector_->NotifyObserversForTesting();
  }

  FakeUpdateEngineClient* fake_update_engine_client_;
  std::unique_ptr<MockBaseScreenDelegate> mock_base_screen_delegate_;
  std::unique_ptr<MockNetworkErrorView> mock_network_error_view_;
  std::unique_ptr<MockErrorScreen> mock_error_screen_;
  UpdateScreen* update_screen_;
  NetworkPortalDetectorTestImpl* network_portal_detector_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UpdateScreenTest);
};

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestBasic) {
  ASSERT_TRUE(update_screen_->view_ != NULL);
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestNoUpdate) {
  update_screen_->SetIgnoreIdleStatus(true);
  UpdateEngineClient::Status status;
  status.status = UpdateEngineClient::UPDATE_STATUS_IDLE;
  update_screen_->UpdateStatusChanged(status);
  status.status = UpdateEngineClient::UPDATE_STATUS_CHECKING_FOR_UPDATE;
  update_screen_->UpdateStatusChanged(status);
  status.status = UpdateEngineClient::UPDATE_STATUS_IDLE;
  // GetLastStatus() will be called via ExitUpdate() called from
  // UpdateStatusChanged().
  fake_update_engine_client_->set_default_status(status);

  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);
  update_screen_->UpdateStatusChanged(status);
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestUpdateAvailable) {
  update_screen_->is_ignore_update_deadlines_ = true;

  UpdateEngineClient::Status status;
  status.status = UpdateEngineClient::UPDATE_STATUS_UPDATE_AVAILABLE;
  status.new_version = "latest and greatest";
  update_screen_->UpdateStatusChanged(status);

  status.status = UpdateEngineClient::UPDATE_STATUS_DOWNLOADING;
  status.download_progress = 0.0;
  update_screen_->UpdateStatusChanged(status);

  status.download_progress = 0.5;
  update_screen_->UpdateStatusChanged(status);

  status.download_progress = 1.0;
  update_screen_->UpdateStatusChanged(status);

  status.status = UpdateEngineClient::UPDATE_STATUS_VERIFYING;
  update_screen_->UpdateStatusChanged(status);

  status.status = UpdateEngineClient::UPDATE_STATUS_FINALIZING;
  update_screen_->UpdateStatusChanged(status);

  status.status = UpdateEngineClient::UPDATE_STATUS_UPDATED_NEED_REBOOT;
  update_screen_->UpdateStatusChanged(status);
  // UpdateStatusChanged(status) calls RebootAfterUpdate().
  EXPECT_EQ(1, fake_update_engine_client_->reboot_after_update_call_count());
  // Check that OOBE will resume back at this screen.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(StartupUtils::IsOobeCompleted());
  EXPECT_EQ(update_screen_->screen_id(),
            GetOobeScreenFromName(g_browser_process->local_state()->GetString(
                prefs::kOobeScreenPending)));
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestErrorIssuingUpdateCheck) {
  // First, cancel the update that is already in progress.
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);
  update_screen_->CancelUpdate();

  fake_update_engine_client_->set_update_check_result(
      chromeos::UpdateEngineClient::UPDATE_RESULT_FAILED);
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_ERROR_CHECKING_FOR_UPDATE, _))
      .Times(1);
  update_screen_->StartNetworkCheck();
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestErrorCheckingForUpdate) {
  UpdateEngineClient::Status status;
  status.status = UpdateEngineClient::UPDATE_STATUS_ERROR;
  // GetLastStatus() will be called via ExitUpdate() called from
  // UpdateStatusChanged().
  fake_update_engine_client_->set_default_status(status);

  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_ERROR_CHECKING_FOR_UPDATE, _))
      .Times(1);
  update_screen_->UpdateStatusChanged(status);
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestErrorUpdating) {
  UpdateEngineClient::Status status;
  status.status = UpdateEngineClient::UPDATE_STATUS_UPDATE_AVAILABLE;
  status.new_version = "latest and greatest";
  // GetLastStatus() will be called via ExitUpdate() called from
  // UpdateStatusChanged().
  fake_update_engine_client_->set_default_status(status);

  update_screen_->UpdateStatusChanged(status);

  status.status = UpdateEngineClient::UPDATE_STATUS_ERROR;
  // GetLastStatus() will be called via ExitUpdate() called from
  // UpdateStatusChanged().
  fake_update_engine_client_->set_default_status(status);

  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_ERROR_UPDATING, _))
      .Times(1);
  update_screen_->UpdateStatusChanged(status);
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestTemproraryOfflineNetwork) {
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);
  update_screen_->CancelUpdate();

  // Change ethernet state to portal.
  NetworkPortalDetector::CaptivePortalState portal_state;
  portal_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL;
  portal_state.response_code = 200;
  SetDetectionResults(kStubEthernetGuid, portal_state);

  // Update screen will delay error message about portal state because
  // ethernet is behind captive portal.
  EXPECT_CALL(*mock_error_screen_,
              MockSetUIState(NetworkError::UI_STATE_UPDATE))
      .Times(1);
  EXPECT_CALL(
      *mock_error_screen_,
      MockSetErrorState(NetworkError::ERROR_STATE_PORTAL, std::string()))
      .Times(1);
  EXPECT_CALL(*mock_error_screen_, MockFixCaptivePortal()).Times(1);
  EXPECT_CALL(*mock_base_screen_delegate_, ShowErrorScreen()).Times(1);

  update_screen_->StartNetworkCheck();

  // Force timer expiration.
  {
    base::Closure timed_callback =
        update_screen_->GetErrorMessageTimerForTesting().user_task();
    ASSERT_FALSE(timed_callback.is_null());
    update_screen_->GetErrorMessageTimerForTesting().Reset();
    timed_callback.Run();
  }

  NetworkPortalDetector::CaptivePortalState online_state;
  online_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE;
  online_state.response_code = 204;
  SetDetectionResults(kStubEthernetGuid, online_state);

  // Second notification from portal detector will be about online state,
  // so update screen will hide error message and proceed to update.
  EXPECT_CALL(*mock_base_screen_delegate_, HideErrorScreen(update_screen_))
      .Times(1);
  fake_update_engine_client_->set_update_check_result(
      chromeos::UpdateEngineClient::UPDATE_RESULT_FAILED);

  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_ERROR_CHECKING_FOR_UPDATE, _))
      .Times(1);

  NotifyPortalDetectionCompleted();
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestTwoOfflineNetworks) {
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);
  update_screen_->CancelUpdate();

  // Change ethernet state to portal.
  NetworkPortalDetector::CaptivePortalState portal_state;
  portal_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL;
  portal_state.response_code = 200;
  SetDetectionResults(kStubEthernetGuid, portal_state);

  // Update screen will delay error message about portal state because
  // ethernet is behind captive portal.
  EXPECT_CALL(*mock_error_screen_,
              MockSetUIState(NetworkError::UI_STATE_UPDATE))
      .Times(1);
  EXPECT_CALL(
      *mock_error_screen_,
      MockSetErrorState(NetworkError::ERROR_STATE_PORTAL, std::string()))
      .Times(1);
  EXPECT_CALL(*mock_error_screen_, MockFixCaptivePortal()).Times(1);
  EXPECT_CALL(*mock_base_screen_delegate_, ShowErrorScreen()).Times(1);

  update_screen_->StartNetworkCheck();

  // Force timer expiration.
  {
    base::Closure timed_callback =
        update_screen_->GetErrorMessageTimerForTesting().user_task();
    ASSERT_FALSE(timed_callback.is_null());
    update_screen_->GetErrorMessageTimerForTesting().Reset();
    timed_callback.Run();
  }

  // Change active network to the wifi behind proxy.
  NetworkPortalDetector::CaptivePortalState proxy_state;
  proxy_state.status =
      NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PROXY_AUTH_REQUIRED;
  proxy_state.response_code = -1;
  SetDefaultNetwork(kStubWifiGuid);
  SetDetectionResults(kStubWifiGuid, proxy_state);

  // Update screen will show message about proxy error because wifie
  // network requires proxy authentication.
  EXPECT_CALL(*mock_error_screen_,
              MockSetErrorState(NetworkError::ERROR_STATE_PROXY, std::string()))
      .Times(1);

  NotifyPortalDetectionCompleted();
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestVoidNetwork) {
  SetDefaultNetwork(std::string());

  // Cancels pending update request.
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);
  update_screen_->CancelUpdate();

  // First portal detection attempt returns NULL network and undefined
  // results, so detection is restarted.
  EXPECT_CALL(*mock_error_screen_, MockSetUIState(_)).Times(Exactly(0));
  EXPECT_CALL(*mock_error_screen_, MockSetErrorState(_, _)).Times(Exactly(0));
  EXPECT_CALL(*mock_base_screen_delegate_, ShowErrorScreen()).Times(Exactly(0));
  update_screen_->StartNetworkCheck();

  // Second portal detection also returns NULL network and undefined
  // results.  In this case, offline message should be displayed.
  EXPECT_CALL(*mock_error_screen_,
              MockSetUIState(NetworkError::UI_STATE_UPDATE))
      .Times(1);
  EXPECT_CALL(
      *mock_error_screen_,
      MockSetErrorState(NetworkError::ERROR_STATE_OFFLINE, std::string()))
      .Times(1);
  EXPECT_CALL(*mock_base_screen_delegate_, ShowErrorScreen()).Times(1);
  base::RunLoop().RunUntilIdle();
  NotifyPortalDetectionCompleted();
}

IN_PROC_BROWSER_TEST_F(UpdateScreenTest, TestAPReselection) {
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_NOUPDATE, _))
      .Times(1);
  update_screen_->CancelUpdate();

  // Change ethernet state to portal.
  NetworkPortalDetector::CaptivePortalState portal_state;
  portal_state.status = NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL;
  portal_state.response_code = 200;
  SetDetectionResults(kStubEthernetGuid, portal_state);

  // Update screen will delay error message about portal state because
  // ethernet is behind captive portal.
  EXPECT_CALL(*mock_error_screen_,
              MockSetUIState(NetworkError::UI_STATE_UPDATE))
      .Times(1);
  EXPECT_CALL(
      *mock_error_screen_,
      MockSetErrorState(NetworkError::ERROR_STATE_PORTAL, std::string()))
      .Times(1);
  EXPECT_CALL(*mock_error_screen_, MockFixCaptivePortal()).Times(1);
  EXPECT_CALL(*mock_base_screen_delegate_, ShowErrorScreen()).Times(1);

  update_screen_->StartNetworkCheck();

  // Force timer expiration.
  {
    base::Closure timed_callback =
        update_screen_->GetErrorMessageTimerForTesting().user_task();
    ASSERT_FALSE(timed_callback.is_null());
    update_screen_->GetErrorMessageTimerForTesting().Reset();
    timed_callback.Run();
  }

  // User re-selects the same network manually. In this case, hide
  // offline message and skip network check. Since ethernet is still
  // behind portal, update engine fails to update.
  EXPECT_CALL(*mock_base_screen_delegate_, HideErrorScreen(update_screen_))
      .Times(1);
  fake_update_engine_client_->set_update_check_result(
      chromeos::UpdateEngineClient::UPDATE_RESULT_FAILED);
  EXPECT_CALL(*mock_base_screen_delegate_,
              OnExit(_, ScreenExitCode::UPDATE_ERROR_CHECKING_FOR_UPDATE, _))
      .Times(1);

  update_screen_->OnConnectRequested();
  base::RunLoop().RunUntilIdle();
}

}  // namespace chromeos
