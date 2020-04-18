// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_android.h"

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class MockMemoryMonitorAndroidDelegate : public MemoryMonitorAndroid::Delegate {
 public:
  MockMemoryMonitorAndroidDelegate() {}
  ~MockMemoryMonitorAndroidDelegate() override {}

  using MemoryInfo = MemoryMonitorAndroid::MemoryInfo;

  void SetMemoryInfo(const MemoryInfo& info) {
    memcpy(&memory_info_, &info, sizeof(memory_info_));
  }

  void GetMemoryInfo(MemoryInfo* out) override {
    memcpy(out, &memory_info_, sizeof(memory_info_));
  }

 private:
  MemoryInfo memory_info_;

  DISALLOW_COPY_AND_ASSIGN(MockMemoryMonitorAndroidDelegate);
};

class MemoryMonitorAndroidTest : public testing::Test {
 public:
  MemoryMonitorAndroidTest() : monitor_(MemoryMonitorAndroid::Create()) {
    auto mock_delegate = base::WrapUnique(new MockMemoryMonitorAndroidDelegate);
    mocked_monitor_.reset(
        new MemoryMonitorAndroid(std::move(mock_delegate)));
  }

 protected:
  static const int kMBShift = 20;

  MockMemoryMonitorAndroidDelegate* mock_delegate() {
    return static_cast<MockMemoryMonitorAndroidDelegate*>(
        mocked_monitor_->delegate());
  }

  std::unique_ptr<MemoryMonitorAndroid> monitor_;
  std::unique_ptr<MemoryMonitorAndroid> mocked_monitor_;
};

TEST_F(MemoryMonitorAndroidTest, GetMemoryInfo) {
  MemoryMonitorAndroid::MemoryInfo info;
  monitor_->GetMemoryInfo(&info);
  EXPECT_GT(info.avail_mem, 0);
  EXPECT_GT(info.threshold, 0);
  EXPECT_GT(info.total_mem, 0);
}

TEST_F(MemoryMonitorAndroidTest, GetFreeMemoryUntilCriticalMB) {
  MemoryMonitorAndroid::MemoryInfo info = {
    .avail_mem = 100 << kMBShift,
    .low_memory = false,
    .threshold = 80 << kMBShift,
    .total_mem = 150 << kMBShift,
  };
  mock_delegate()->SetMemoryInfo(info);
  EXPECT_EQ(20, mocked_monitor_->GetFreeMemoryUntilCriticalMB());
}

}  // namespace content
