// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_monitor_win.h"

#include "content/browser/memory/test_memory_monitor.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A delegate that allows mocking the various inputs to MemoryMonitorWin.
class TestMemoryMonitorWinDelegate : public TestMemoryMonitorDelegate {
 public:
  TestMemoryMonitorWinDelegate() {}

  void SetFreeMemoryKB(int free_memory_kb) {
    mem_info_.avail_phys = free_memory_kb;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestMemoryMonitorWinDelegate);
};

class TestMemoryMonitorWin : public MemoryMonitorWin {
 public:
  using MemoryMonitorWin::IsLargeMemory;
  using MemoryMonitorWin::GetTargetFreeMB;
};

static const int kKBperMB = 1024;

}  // namespace

class MemoryMonitorWinTest : public testing::Test {
 public:
  TestMemoryMonitorWinDelegate delegate_;
  std::unique_ptr<MemoryMonitorWin> monitor_;
};

TEST_F(MemoryMonitorWinTest, IsLargeMemory) {
  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB  - 1);
  EXPECT_FALSE(TestMemoryMonitorWin::IsLargeMemory(&delegate_));
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB);
  EXPECT_TRUE(TestMemoryMonitorWin::IsLargeMemory(&delegate_));
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB + 100);
  EXPECT_TRUE(TestMemoryMonitorWin::IsLargeMemory(&delegate_));
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();
}

TEST_F(MemoryMonitorWinTest, GetTargetFreeMB) {
  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB  - 1);
  EXPECT_EQ(MemoryMonitorWin::kSmallMemoryTargetFreeMB,
            TestMemoryMonitorWin::GetTargetFreeMB(&delegate_));
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB);
  EXPECT_EQ(MemoryMonitorWin::kLargeMemoryTargetFreeMB,
            TestMemoryMonitorWin::GetTargetFreeMB(&delegate_));
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB + 100);
  EXPECT_EQ(MemoryMonitorWin::kLargeMemoryTargetFreeMB,
            TestMemoryMonitorWin::GetTargetFreeMB(&delegate_));
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();
}

TEST_F(MemoryMonitorWinTest, Create) {
  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB  - 1);
  monitor_ = MemoryMonitorWin::Create(&delegate_);
  EXPECT_EQ(MemoryMonitorWin::kSmallMemoryTargetFreeMB,
            monitor_->target_free_mb());
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB);
  monitor_ = MemoryMonitorWin::Create(&delegate_);
  EXPECT_EQ(MemoryMonitorWin::kLargeMemoryTargetFreeMB,
            monitor_->target_free_mb());
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetTotalMemoryKB(
        MemoryMonitorWin::kLargeMemoryThresholdMB * kKBperMB + 100);
  monitor_ = MemoryMonitorWin::Create(&delegate_);
  EXPECT_EQ(MemoryMonitorWin::kLargeMemoryTargetFreeMB,
            monitor_->target_free_mb());
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();
}

TEST_F(MemoryMonitorWinTest, Constructor) {
  monitor_.reset(new MemoryMonitorWin(&delegate_, 100));
  EXPECT_EQ(100, monitor_->target_free_mb());
  EXPECT_EQ(0u, delegate_.calls());

  monitor_.reset(new MemoryMonitorWin(&delegate_, 387));
  EXPECT_EQ(387, monitor_->target_free_mb());
  EXPECT_EQ(0u, delegate_.calls());
}

TEST_F(MemoryMonitorWinTest, GetFreeMemoryUntilCriticalMB) {
  monitor_.reset(new MemoryMonitorWin(&delegate_, 100));
  EXPECT_EQ(100, monitor_->target_free_mb());
  EXPECT_EQ(0u, delegate_.calls());

  delegate_.SetFreeMemoryKB(200 * kKBperMB);
  EXPECT_EQ(100, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();

  delegate_.SetFreeMemoryKB(50 * kKBperMB);
  EXPECT_EQ(-50, monitor_->GetFreeMemoryUntilCriticalMB());
  EXPECT_EQ(1u, delegate_.calls());
  delegate_.ResetCalls();
}

}  // namespace content
