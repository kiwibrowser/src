// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/device/bluetooth/le/le_scan_manager_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/device/bluetooth/bluetooth_util.h"
#include "chromecast/device/bluetooth/le/remote_characteristic.h"
#include "chromecast/device/bluetooth/le/remote_descriptor.h"
#include "chromecast/device/bluetooth/le/remote_device.h"
#include "chromecast/device/bluetooth/le/remote_service.h"
#include "chromecast/device/bluetooth/shlib/mock_gatt_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace chromecast {
namespace bluetooth {

namespace {

const bluetooth_v2_shlib::Addr kTestAddr1 = {
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};
const bluetooth_v2_shlib::Addr kTestAddr2 = {
    {0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}};

// This hack is needed because base::BindOnce does not support capture lambdas.
template <typename T>
void CopyResult(T* out, T in) {
  *out = in;
}

class FakeLeScannerImpl : public bluetooth_v2_shlib::LeScannerImpl {
 public:
  FakeLeScannerImpl() {}
  ~FakeLeScannerImpl() override = default;

  // bluetooth_v2_shlib::LeScannerImpl implementation:
  bool IsSupported() override { return true; }
  void SetDelegate(bluetooth_v2_shlib::LeScanner::Delegate* delegate) override {
  }
  bool StartScan() override { return true; }
  bool StopScan() override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeLeScannerImpl);
};

class MockLeScanManagerObserver : public LeScanManager::Observer {
 public:
  MOCK_METHOD1(OnScanEnableChanged, void(bool enabled));
  MOCK_METHOD1(OnNewScanResult, void(LeScanResult result));
};

class LeScanManagerTest : public ::testing::Test {
 protected:
  LeScanManagerTest()
      : io_task_runner_(base::CreateSingleThreadTaskRunnerWithTraits(
            {base::TaskPriority::BACKGROUND, base::MayBlock()})),
        le_scan_manager_(&fake_le_scan_manager_impl_) {}
  ~LeScanManagerTest() override {}

  // testing::Test implementation:
  void SetUp() override {
    le_scan_manager_.Initialize(io_task_runner_);
    le_scan_manager_.AddObserver(&mock_observer_);
    scoped_task_environment_.RunUntilIdle();
  }

  void TearDown() override {
    le_scan_manager_.RemoveObserver(&mock_observer_);
    le_scan_manager_.Finalize();
  }

  bluetooth_v2_shlib::LeScanner::Delegate* delegate() {
    return &le_scan_manager_;
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  FakeLeScannerImpl fake_le_scan_manager_impl_;
  LeScanManagerImpl le_scan_manager_;
  MockLeScanManagerObserver mock_observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LeScanManagerTest);
};

}  // namespace

TEST_F(LeScanManagerTest, TestSetScanEnable) {
  bool enabled = false;
  EXPECT_CALL(mock_observer_, OnScanEnableChanged(true));

  // Enable the LE scan. We expect the observer to be updated and the callback
  // to be called with success=true.
  le_scan_manager_.SetScanEnable(true, /* enable */
                                 base::BindOnce(&CopyResult<bool>, &enabled));

  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(enabled);
}

TEST_F(LeScanManagerTest, TestGetScanResultsEmpty) {
  std::vector<LeScanResult> results;

  // Get asynchronous scan results. The result should be empty.
  le_scan_manager_.GetScanResults(
      base::BindOnce(&CopyResult<std::vector<LeScanResult>>, &results));

  scoped_task_environment_.RunUntilIdle();
  ASSERT_EQ(0u, results.size());
}

TEST_F(LeScanManagerTest, TestGetScanResults) {
  // Simulate some scan results.
  bluetooth_v2_shlib::LeScanner::ScanResult raw_scan_result;
  raw_scan_result.addr = kTestAddr1;
  raw_scan_result.rssi = 1234;

  EXPECT_CALL(mock_observer_, OnNewScanResult(_));
  delegate()->OnScanResult(raw_scan_result);
  scoped_task_environment_.RunUntilIdle();

  std::vector<LeScanResult> results;
  // Get asynchronous scan results.
  le_scan_manager_.GetScanResults(
      base::BindOnce(&CopyResult<std::vector<LeScanResult>>, &results));

  scoped_task_environment_.RunUntilIdle();

  ASSERT_EQ(1u, results.size());
  ASSERT_EQ(kTestAddr1, results[0].addr);
  ASSERT_EQ(1234, results[0].rssi);
}

TEST_F(LeScanManagerTest, TestGetScanResultsWithService) {
  EXPECT_CALL(mock_observer_, OnNewScanResult(_)).Times(2);

  // Add a scan result with service 0x4444.
  bluetooth_v2_shlib::LeScanner::ScanResult raw_scan_result;
  raw_scan_result.addr = kTestAddr1;
  raw_scan_result.adv_data = {0x03, 0x02, 0x44, 0x44};
  raw_scan_result.rssi = 1234;
  delegate()->OnScanResult(raw_scan_result);

  // Add a scan result with service 0x5555.
  raw_scan_result.addr = kTestAddr2;
  raw_scan_result.adv_data = {0x03, 0x02, 0x55, 0x55};
  raw_scan_result.rssi = 1234;
  delegate()->OnScanResult(raw_scan_result);

  scoped_task_environment_.RunUntilIdle();

  // Get asynchronous scan results for results with service 0x4444.
  std::vector<LeScanResult> results;
  le_scan_manager_.GetScanResults(
      base::BindOnce(&CopyResult<std::vector<LeScanResult>>, &results),
      ScanFilter::From16bitUuid(0x4444));
  scoped_task_environment_.RunUntilIdle();

  ASSERT_EQ(1u, results.size());
  ASSERT_EQ(kTestAddr1, results[0].addr);
  ASSERT_EQ(std::vector<uint8_t>({0x44, 0x44}),
            results[0].type_to_data[0x02][0]);
  ASSERT_EQ(1234, results[0].rssi);

  // Get asynchronous scan results for results with service 0x5555.
  le_scan_manager_.GetScanResults(
      base::BindOnce(&CopyResult<std::vector<LeScanResult>>, &results),
      ScanFilter::From16bitUuid(0x5555));
  scoped_task_environment_.RunUntilIdle();

  ASSERT_EQ(1u, results.size());
  ASSERT_EQ(kTestAddr2, results[0].addr);
  ASSERT_EQ(std::vector<uint8_t>({0x55, 0x55}),
            results[0].type_to_data[0x02][0]);
  ASSERT_EQ(1234, results[0].rssi);

  // Get asynchronous scan results for results with service 0x6666.
  le_scan_manager_.GetScanResults(
      base::BindOnce(&CopyResult<std::vector<LeScanResult>>, &results),
      ScanFilter::From16bitUuid(0x6666));
  scoped_task_environment_.RunUntilIdle();

  ASSERT_EQ(0u, results.size());
}

TEST_F(LeScanManagerTest, TestGetScanResultsSortedByRssi) {
  EXPECT_CALL(mock_observer_, OnNewScanResult(_)).Times(3);

  // Add a scan result with service 0x4444.
  bluetooth_v2_shlib::LeScanner::ScanResult raw_scan_result;
  raw_scan_result.addr = kTestAddr1;
  raw_scan_result.adv_data = {0x03, 0x02, 0x44, 0x44};
  raw_scan_result.rssi = 1;
  delegate()->OnScanResult(raw_scan_result);

  // Add a scan result with service 0x5555.
  raw_scan_result.addr = kTestAddr2;
  raw_scan_result.adv_data = {0x03, 0x02, 0x55, 0x55};
  raw_scan_result.rssi = 3;
  delegate()->OnScanResult(raw_scan_result);

  // Add a scan result with service 0x5555.
  raw_scan_result.addr = kTestAddr1;
  raw_scan_result.adv_data = {0x03, 0x02, 0x55, 0x55};
  raw_scan_result.rssi = 2;
  delegate()->OnScanResult(raw_scan_result);

  scoped_task_environment_.RunUntilIdle();

  std::vector<LeScanResult> results;
  // Get asynchronous scan results.
  le_scan_manager_.GetScanResults(
      base::BindOnce(&CopyResult<std::vector<LeScanResult>>, &results));

  scoped_task_environment_.RunUntilIdle();

  ASSERT_EQ(3u, results.size());
  EXPECT_EQ(kTestAddr2, results[0].addr);
  EXPECT_EQ(3, results[0].rssi);
  EXPECT_EQ(kTestAddr1, results[1].addr);
  EXPECT_EQ(2, results[1].rssi);
  EXPECT_EQ(kTestAddr1, results[2].addr);
  EXPECT_EQ(1, results[2].rssi);
}

TEST_F(LeScanManagerTest, TestOnNewScanResult) {
  LeScanResult result;
  ON_CALL(mock_observer_, OnNewScanResult(_))
      .WillByDefault(
          Invoke([&result](LeScanResult result_in) { result = result_in; }));

  // Add a scan result with service 0x4444.
  bluetooth_v2_shlib::LeScanner::ScanResult raw_scan_result;
  raw_scan_result.addr = kTestAddr1;
  raw_scan_result.adv_data = {0x03, 0x02, 0x44, 0x44};
  raw_scan_result.rssi = 1;
  delegate()->OnScanResult(raw_scan_result);
  scoped_task_environment_.RunUntilIdle();

  // Ensure that the observer was notified.
  ASSERT_EQ(kTestAddr1, result.addr);
  ASSERT_EQ(std::vector<uint8_t>({0x44, 0x44}), result.type_to_data[0x02][0]);
  ASSERT_EQ(1, result.rssi);
}

TEST_F(LeScanManagerTest, TestOnScanEnableChanged) {
  bool enabled = false;
  ON_CALL(mock_observer_, OnScanEnableChanged(_))
      .WillByDefault(
          Invoke([&enabled](bool enabled_in) { enabled = enabled_in; }));

  // Enable scanning.
  le_scan_manager_.SetScanEnable(true /* enable */, base::DoNothing());
  scoped_task_environment_.RunUntilIdle();
  ASSERT_TRUE(enabled);

  // Disable scanning.
  le_scan_manager_.SetScanEnable(false /* enable */, base::DoNothing());
  scoped_task_environment_.RunUntilIdle();
  ASSERT_FALSE(enabled);
}

}  // namespace bluetooth
}  // namespace chromecast
