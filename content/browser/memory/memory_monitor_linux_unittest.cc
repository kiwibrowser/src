// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_linux.h"

#include "content/browser/memory/test_memory_monitor.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A delegate that allows mocking the various inputs to MemoryMonitorLinux.
class TestMemoryMonitorLinuxDelegate : public TestMemoryMonitorDelegate {
 public:
  TestMemoryMonitorLinuxDelegate() {}

  void SetAvailableMemoryKB(int available_memory_kb) {
    // If this is set, other "free" values are ignored.
    mem_info_.available = available_memory_kb;
  }

  void SetFreeMemoryKB(int free_kb, int cached_kb, int buffers_kb) {
    mem_info_.free = free_kb;
    mem_info_.cached = cached_kb;
    mem_info_.buffers = buffers_kb;

    // Only if this is zero will the above values be used.
    mem_info_.available = 0;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestMemoryMonitorLinuxDelegate);
};

class TestMemoryMonitorLinux : public MemoryMonitorLinux {};

static const int kKBperMB = 1024;

}  // namespace

class MemoryMonitorLinuxTest : public testing::Test {
 public:
  TestMemoryMonitorLinuxDelegate delegate_;
  std::unique_ptr<MemoryMonitorLinux> monitor_;
};

TEST_F(MemoryMonitorLinuxTest, Create) {
  delegate_.SetTotalMemoryKB(100000 * kKBperMB);
  monitor_ = MemoryMonitorLinux::Create(&delegate_);
  EXPECT_EQ(0U, delegate_.calls());
}

TEST_F(MemoryMonitorLinuxTest, GetFreeMemoryUntilCriticalMB) {
  delegate_.SetTotalMemoryKB(1000 * kKBperMB);

  monitor_.reset(new MemoryMonitorLinux(&delegate_));
  EXPECT_EQ(0u, delegate_.calls());

  delegate_.SetAvailableMemoryKB(200 * kKBperMB);
  EXPECT_EQ(200, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(1U, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetFreeMemoryKB(64 * kKBperMB, 32 * kKBperMB, 16 * kKBperMB);
  EXPECT_EQ(64, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(1U, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetFreeMemoryKB(0, 0, 0);
  EXPECT_EQ(0, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(1U, delegate_.calls());
  delegate_.ResetCalls();
}

}  // namespace content
