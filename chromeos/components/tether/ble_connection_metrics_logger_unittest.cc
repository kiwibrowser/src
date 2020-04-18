// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_connection_metrics_logger.h"

#include "base/memory/ptr_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

constexpr base::TimeDelta kReceiveAdvertisementTime =
    base::TimeDelta::FromSeconds(1);
constexpr base::TimeDelta kStatusConnectedTime =
    base::TimeDelta::FromSeconds(3);
constexpr base::TimeDelta kStatusAuthenticatedTime =
    base::TimeDelta::FromSeconds(7);

static const size_t kNumFailedConnectionAttempts = 6u;

std::string kDeviceId1 = "deviceId1";
std::string kDeviceId2 = "deviceId2";
std::string kDeviceId3 = "deviceId3";

class BleConnectionMetricsLoggerTest : public testing::Test {
 protected:
  BleConnectionMetricsLoggerTest() {}

  void SetUp() override {
    ble_connection_metrics_logger_ =
        base::WrapUnique(new BleConnectionMetricsLogger());

    test_clock_.SetNow(base::Time::UnixEpoch());
    ble_connection_metrics_logger_->SetClockForTesting(&test_clock_);
  }

  void AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      const std::string& device_id,
      bool is_background_advertisement,
      size_t num_previous_devices = 0u) {
    size_t num_total_devices = num_previous_devices + 1;

    ble_connection_metrics_logger_->OnConnectionAttemptStarted(device_id);

    ReceiveAdvertisementAndAdvanceClock(device_id, is_background_advertisement);

    if (is_background_advertisement) {
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance."
          "StartScanToReceiveAdvertisementDuration.Background",
          kReceiveAdvertisementTime, num_total_devices);
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.Performance."
          "StartScanToReceiveAdvertisementDuration",
          0);
    } else {
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.Performance."
          "StartScanToReceiveAdvertisementDuration.Background",
          0);
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance."
          "StartScanToReceiveAdvertisementDuration",
          kReceiveAdvertisementTime, num_total_devices);
    }

    ConnectAndAdvanceClock(device_id, is_background_advertisement);

    if (is_background_advertisement) {
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance."
          "ReceiveAdvertisementToConnectionDuration.Background",
          kStatusConnectedTime, num_total_devices);
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance.StartScanToConnectionDuration."
          "Background",
          kReceiveAdvertisementTime + kStatusConnectedTime, num_total_devices);

      histogram_tester_.ExpectTotalCount(
          "InstantTethering.Performance."
          "ReceiveAdvertisementToConnectionDuration",
          0);
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.Performance.AdvertisementToConnectionDuration", 0);
    } else {
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.Performance."
          "ReceiveAdvertisementToConnectionDuration.Background",
          0);
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.Performance.StartScanToConnectionDuration."
          "Background",
          0);

      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance."
          "ReceiveAdvertisementToConnectionDuration",
          kStatusConnectedTime, num_total_devices);
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance.AdvertisementToConnectionDuration",
          kReceiveAdvertisementTime + kStatusConnectedTime, num_total_devices);
    }

    CreateSecureChannelAndAdvanceClock(device_id, is_background_advertisement);

    if (is_background_advertisement) {
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance.ConnectionToAuthenticationDuration."
          "Background",
          kStatusAuthenticatedTime, num_total_devices);
    } else {
      histogram_tester_.ExpectTimeBucketCount(
          "InstantTethering.Performance.ConnectionToAuthenticationDuration",
          kStatusAuthenticatedTime, num_total_devices);
    }

    ble_connection_metrics_logger_->OnDeviceDisconnected(
        device_id,
        BleConnectionManager::StateChangeDetail::
            STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED,
        is_background_advertisement);

    VerifyGattConnectionMetrics(
        num_total_devices /* num_expected_successful_gatt_connections */,
        0u /* num_expected_failed_gatt_connections */,
        num_total_devices /* num_expected_successful_effective_gatt_connections
                           */
        ,
        0u /* num_expected_failed_effective_gatt_connections */,
        is_background_advertisement);
  }

  void VerifyGattConnectionMetrics(
      size_t num_expected_successful_gatt_connections,
      size_t num_expected_failed_gatt_connections,
      size_t num_expected_successful_effective_gatt_connections,
      size_t num_expected_failed_effective_gatt_connections,
      bool is_background_advertisement) {
    if (num_expected_successful_gatt_connections > 0u) {
      if (is_background_advertisement) {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt.SuccessRate.Background",
            true, num_expected_successful_gatt_connections);
      } else {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt.SuccessRate", true,
            num_expected_successful_gatt_connections);
      }
    }

    if (num_expected_failed_gatt_connections > 0u) {
      if (is_background_advertisement) {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt.SuccessRate.Background",
            false, num_expected_failed_gatt_connections);
      } else {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt.SuccessRate", false,
            num_expected_failed_gatt_connections);
      }
    }

    if (num_expected_successful_gatt_connections == 0u &&
        num_expected_failed_gatt_connections == 0u) {
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.GattConnectionAttempt.SuccessRate.Background", 0u);
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.GattConnectionAttempt.SuccessRate", 0u);
    }

    if (num_expected_successful_effective_gatt_connections > 0u) {
      if (is_background_advertisement) {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt."
            "EffectiveSuccessRateWithRetries.Background",
            true, num_expected_successful_effective_gatt_connections);
      } else {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt."
            "EffectiveSuccessRateWithRetries",
            true, num_expected_successful_effective_gatt_connections);
      }
    }

    if (num_expected_failed_effective_gatt_connections > 0u) {
      if (is_background_advertisement) {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt."
            "EffectiveSuccessRateWithRetries.Background",
            false, num_expected_failed_effective_gatt_connections);
      } else {
        histogram_tester_.ExpectBucketCount(
            "InstantTethering.GattConnectionAttempt."
            "EffectiveSuccessRateWithRetries",
            false, num_expected_failed_effective_gatt_connections);
      }
    }

    if (num_expected_successful_effective_gatt_connections == 0u &&
        num_expected_failed_effective_gatt_connections == 0u) {
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.GattConnectionAttempt."
          "EffectiveSuccessRateWithRetries.Background",
          0u);
      histogram_tester_.ExpectTotalCount(
          "InstantTethering.GattConnectionAttempt."
          "EffectiveSuccessRateWithRetries",
          0u);
    }
  }

  void FailAllGattConnectionAttemptsAndVerifyMetrics(
      const std::string& device_id,
      bool is_background_advertisement) {
    AttemptGattConnectionsAndThenUnregister(
        device_id, kNumFailedConnectionAttempts,
        false /* succeed_on_additional_last_attempt */,
        is_background_advertisement);

    VerifyGattConnectionMetrics(
        0u /* num_expected_successful_gatt_connections */,
        kNumFailedConnectionAttempts /* num_expected_failed_gatt_connections */,
        0u /* num_expected_successful_effective_gatt_connections */,
        1u /* num_expected_failed_effective_gatt_connections */,
        is_background_advertisement);
  }

  void FailAlmostAllGattConnectionAttemptsAndVerifyMetrics(
      const std::string& device_id,
      bool is_background_advertisement) {
    AttemptGattConnectionsAndThenUnregister(
        device_id, kNumFailedConnectionAttempts,
        true /* succeed_on_additional_last_attempt */,
        is_background_advertisement);

    VerifyGattConnectionMetrics(
        1u /* num_expected_successful_gatt_connections */,
        kNumFailedConnectionAttempts /* num_expected_failed_gatt_connections */,
        1u /* num_expected_successful_effective_gatt_connections */,
        0u /* num_expected_failed_effective_gatt_connections */,
        is_background_advertisement);
  }

  void AttemptGattConnectionsAndThenUnregister(
      const std::string& device_id,
      size_t num_failed_connection_attempts,
      bool succeed_on_additional_last_attempt,
      bool is_background_advertisement) {
    for (size_t i = 0u; i < num_failed_connection_attempts; ++i) {
      ble_connection_metrics_logger_->OnConnectionAttemptStarted(device_id);
      ReceiveAdvertisementAndAdvanceClock(device_id,
                                          is_background_advertisement);

      ble_connection_metrics_logger_->OnDeviceDisconnected(
          device_id,
          BleConnectionManager::StateChangeDetail::
              STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED,
          is_background_advertisement);
    }

    if (succeed_on_additional_last_attempt) {
      // Now succeed on the last possible connection attempt:
      ble_connection_metrics_logger_->OnConnectionAttemptStarted(device_id);
      ReceiveAdvertisementAndAdvanceClock(device_id,
                                          is_background_advertisement);
      ConnectAndAdvanceClock(device_id, is_background_advertisement);
      CreateSecureChannelAndAdvanceClock(device_id,
                                         is_background_advertisement);
    }

    ble_connection_metrics_logger_->OnDeviceDisconnected(
        device_id,
        BleConnectionManager::StateChangeDetail::
            STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED,
        is_background_advertisement);
  }

  void ReceiveAdvertisementAndAdvanceClock(const std::string& device_id,
                                           bool is_background_advertisement) {
    test_clock_.Advance(kReceiveAdvertisementTime);
    ble_connection_metrics_logger_->OnAdvertisementReceived(
        device_id, is_background_advertisement);
  }

  void ConnectAndAdvanceClock(const std::string& device_id,
                              bool is_background_advertisement) {
    test_clock_.Advance(kStatusConnectedTime);
    ble_connection_metrics_logger_->OnConnection(device_id,
                                                 is_background_advertisement);
  }

  void CreateSecureChannelAndAdvanceClock(const std::string& device_id,
                                          bool is_background_advertisement) {
    test_clock_.Advance(kStatusAuthenticatedTime);
    ble_connection_metrics_logger_->OnSecureChannelCreated(
        device_id, is_background_advertisement);
  }

  std::unique_ptr<BleConnectionMetricsLogger> ble_connection_metrics_logger_;

  base::SimpleTestClock test_clock_;
  base::HistogramTester histogram_tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BleConnectionMetricsLoggerTest);
};

TEST_F(BleConnectionMetricsLoggerTest, TestRecordMetrics_OneDevice) {
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId1, false /* is_background_advertisement */);
}

TEST_F(BleConnectionMetricsLoggerTest, TestRecordMetrics_OneDevice_Background) {
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId1, true /* is_background_advertisement */);
}

TEST_F(BleConnectionMetricsLoggerTest, TestRecordMetrics_MultipleDevices) {
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId1, false /* is_background_advertisement */,
      0u /* num_previous_devices */);
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId2, false /* is_background_advertisement */,
      1u /* num_previous_devices */);
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId3, false /* is_background_advertisement */,
      2u /* num_previous_devices */);
}

TEST_F(BleConnectionMetricsLoggerTest,
       TestRecordMetrics_MultipleDevices_Background) {
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId1, true /* is_background_advertisement */,
      0u /* num_previous_devices */);
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId2, true /* is_background_advertisement */,
      1u /* num_previous_devices */);
  AuthenticateAndDisconnectDeviceAndVerifyMetrics(
      kDeviceId3, true /* is_background_advertisement */,
      2u /* num_previous_devices */);
}

TEST_F(BleConnectionMetricsLoggerTest,
       TestRecordMetrics_NeverReceiveAdvertisement) {
  ble_connection_metrics_logger_->OnConnectionAttemptStarted(kDeviceId1);
  ble_connection_metrics_logger_->OnDeviceDisconnected(
      kDeviceId1,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED,
      false /* is_background_advertisement */);

  histogram_tester_.ExpectTotalCount(
      "InstantTethering.Performance."
      "StartScanToReceiveAdvertisementDuration",
      0);
  histogram_tester_.ExpectTotalCount(
      "InstantTethering.Performance."
      "ReceiveAdvertisementToConnectionDuration",
      0);
  histogram_tester_.ExpectTotalCount(
      "InstantTethering.Performance.AdvertisementToConnectionDuration", 0);
}

TEST_F(BleConnectionMetricsLoggerTest, TestRecordMetrics_NeverConnect) {
  ble_connection_metrics_logger_->OnConnectionAttemptStarted(kDeviceId1);
  ReceiveAdvertisementAndAdvanceClock(kDeviceId1,
                                      false /* is_background_advertisement */);
  ble_connection_metrics_logger_->OnDeviceDisconnected(
      kDeviceId1,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED,
      false /* is_background_advertisement */);

  histogram_tester_.ExpectTimeBucketCount(
      "InstantTethering.Performance."
      "StartScanToReceiveAdvertisementDuration",
      kReceiveAdvertisementTime, 1);
  histogram_tester_.ExpectTotalCount(
      "InstantTethering.Performance."
      "ReceiveAdvertisementToConnectionDuration",
      0);
  histogram_tester_.ExpectTotalCount(
      "InstantTethering.Performance.AdvertisementToConnectionDuration", 0);
}

TEST_F(BleConnectionMetricsLoggerTest,
       TestRecordMetrics_NeverCreateSecureChannel) {
  ble_connection_metrics_logger_->OnConnectionAttemptStarted(kDeviceId1);
  ReceiveAdvertisementAndAdvanceClock(kDeviceId1,
                                      false /* is_background_advertisement */);
  ConnectAndAdvanceClock(kDeviceId1, false /* is_background_advertisement */);
  ble_connection_metrics_logger_->OnDeviceDisconnected(
      kDeviceId1,
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED,
      false /* is_background_advertisement */);

  histogram_tester_.ExpectTimeBucketCount(
      "InstantTethering.Performance."
      "StartScanToReceiveAdvertisementDuration",
      kReceiveAdvertisementTime, 1);
  histogram_tester_.ExpectTimeBucketCount(
      "InstantTethering.Performance."
      "ReceiveAdvertisementToConnectionDuration",
      kStatusConnectedTime, 1);
  histogram_tester_.ExpectTimeBucketCount(
      "InstantTethering.Performance.AdvertisementToConnectionDuration",
      kReceiveAdvertisementTime + kStatusConnectedTime, 1);
  histogram_tester_.ExpectTotalCount(
      "InstantTethering.Performance.ConnectionToAuthenticationDuration", 0);
}

TEST_F(BleConnectionMetricsLoggerTest,
       TestRecordMetrics_CannotCompleteGattConnection_RetryLimitReached) {
  FailAllGattConnectionAttemptsAndVerifyMetrics(
      kDeviceId1, false /* is_background_advertisement */);
}

TEST_F(
    BleConnectionMetricsLoggerTest,
    TestRecordMetrics_CannotCompleteGattConnection_RetryLimitReached_Background) {
  FailAllGattConnectionAttemptsAndVerifyMetrics(
      kDeviceId1, true /* is_background_advertisement */);
}

TEST_F(BleConnectionMetricsLoggerTest,
       TestRecordMetrics_CompleteGattConnection_RetryLimitAlmostReached) {
  FailAlmostAllGattConnectionAttemptsAndVerifyMetrics(
      kDeviceId1, false /* is_background_advertisement */);
}

TEST_F(
    BleConnectionMetricsLoggerTest,
    TestRecordMetrics_CompleteGattConnection_RetryLimitAlmostReached_Background) {
  FailAlmostAllGattConnectionAttemptsAndVerifyMetrics(
      kDeviceId1, true /* is_background_advertisement */);
}

}  // namespace tether

}  // namespace chromeos
