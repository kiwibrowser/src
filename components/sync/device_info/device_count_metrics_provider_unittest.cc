// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/device_info/device_count_metrics_provider.h"

#include <string>

#include "base/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "components/sync/device_info/device_info.h"
#include "components/sync/device_info/device_info_tracker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

class FakeTracker : public DeviceInfoTracker {
 public:
  explicit FakeTracker(const int count) : count_(count) {}

  // DeviceInfoTracker
  bool IsSyncing() const override { return false; }
  std::unique_ptr<DeviceInfo> GetDeviceInfo(
      const std::string& client_id) const override {
    return std::unique_ptr<DeviceInfo>();
  }
  std::vector<std::unique_ptr<DeviceInfo>> GetAllDeviceInfo() const override {
    return std::vector<std::unique_ptr<DeviceInfo>>();
  }
  void AddObserver(Observer* observer) override {}
  void RemoveObserver(Observer* observer) override {}
  int CountActiveDevices() const override { return count_; }

 private:
  int count_;
};

}  // namespace

class DeviceCountMetricsProviderTest : public testing::Test {
 public:
  DeviceCountMetricsProviderTest()
      : metrics_provider_(
            base::Bind(&DeviceCountMetricsProviderTest::GetTrackers,
                       base::Unretained(this))) {}

  void AddTracker(const int count) {
    trackers_.push_back(
        std::unique_ptr<DeviceInfoTracker>(new FakeTracker(count)));
  }
  void GetTrackers(std::vector<const DeviceInfoTracker*>* trackers) {
    for (const auto& tracker : trackers_) {
      trackers->push_back(tracker.get());
    }
  }

  void TestProvider(int expected_device_count) {
    base::HistogramTester histogram_tester;
    metrics_provider_.ProvideCurrentSessionData(nullptr);
    histogram_tester.ExpectUniqueSample("Sync.DeviceCount",
                                        expected_device_count, 1);
  }

 private:
  DeviceCountMetricsProvider metrics_provider_;
  std::vector<std::unique_ptr<DeviceInfoTracker>> trackers_;
};

namespace {

TEST_F(DeviceCountMetricsProviderTest, NoTrackers) {
  TestProvider(0);
}

TEST_F(DeviceCountMetricsProviderTest, SingleTracker) {
  AddTracker(2);
  TestProvider(2);
}

TEST_F(DeviceCountMetricsProviderTest, MultipileTrackers) {
  AddTracker(1);
  AddTracker(5);
  AddTracker(-123);
  AddTracker(0);
  TestProvider(5);
}

TEST_F(DeviceCountMetricsProviderTest, OnlyNegative) {
  AddTracker(-123);
  TestProvider(0);
}

TEST_F(DeviceCountMetricsProviderTest, VeryLarge) {
  AddTracker(123456789);
  TestProvider(100);
}

}  // namespace

}  // namespace syncer
